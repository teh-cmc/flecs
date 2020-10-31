#include "private_api.h"

#define PAGE_COUNT (4096)
#define PAGE_SIZE(paged) (paged->column_count * ECS_SIZEOF(void*))
#define PAGE(index) ((int32_t)index >> 12)
#define OFFSET(index) ((int32_t)index & 0xFFF)
#define DATA(array, size, offset) (ECS_OFFSET(array, size * offset))

static
ecs_page_t* page_new(
    ecs_paged_t *paged,
    int32_t page_index)
{
    int32_t count = ecs_vector_count(paged->pages);
    ecs_page_t *pages;
    ecs_size_t page_size = PAGE_SIZE(paged);

    if (count <= page_index) {
        _ecs_vector_set_count(&paged->pages, page_size, page_index + 1);
        pages = _ecs_vector_first(paged->pages, page_size);
        ecs_os_memset( ECS_OFFSET(pages, count * page_size), 0, 
            (1 + page_index - count) * page_size);
    } else {
        pages = _ecs_vector_first(paged->pages, page_size);
    }

    ecs_assert(pages != NULL, ECS_INTERNAL_ERROR);

    ecs_page_t *result = ECS_OFFSET(pages, page_index * page_size);

    /* Initialize the data arrays with zero's to guarantee that data is 
     * always initialized. When an entry is removed, data is reset back to
     * zero. Initialize now, as this can take advantage of calloc. */
    int32_t i;
    for (i = 0; i < paged->column_count; i ++) {
        ecs_assert(paged->column_sizes[i] != 0, ECS_INTERNAL_ERROR);
        result[i] = ecs_os_calloc(paged->column_sizes[i] * PAGE_COUNT);
        ecs_assert(result[i] != NULL, ECS_OUT_OF_MEMORY);
    }
    
    return result;
}

static
ecs_page_t* page_get(
    const ecs_paged_t *paged,
    int32_t page_index)
{
    ecs_assert(page_index >= 0, ECS_INTERNAL_ERROR);
    ecs_page_t *result = _ecs_vector_get(
        paged->pages, PAGE_SIZE(paged), page_index);
    if (result && result[0]) {
        return result;
    } else {
        return NULL;
    }
}

static
ecs_page_t* page_ensure(
    ecs_paged_t *paged,
    int32_t page_index)
{
    ecs_page_t *page = page_get(paged, page_index);
    if (page) {
        return page;
    }

    return page_new(paged, page_index);
}

static
void page_free(
    ecs_paged_t *paged,
    ecs_page_t *page)
{
    int32_t i;
    for (i = 0; i < paged->column_count; i ++) {
        ecs_os_free(page[i]);
    }
}

void ecs_paged_init(
    ecs_paged_t *paged,
    int32_t column_count,
    ecs_size_t *column_sizes)
{
    ecs_size_t array_size = column_count * ECS_SIZEOF(ecs_size_t);
    paged->column_sizes = ecs_os_malloc(array_size);
    ecs_os_memcpy(paged->column_sizes, column_sizes, array_size);
    paged->column_count = column_count;
}

ecs_paged_t* ecs_paged_new(
    int32_t column_count,
    ecs_size_t *column_sizes)
{
    ecs_paged_t *result = ecs_os_calloc(sizeof(ecs_paged_t));
    ecs_paged_init(result, column_count, column_sizes);
    return result;
}

void ecs_paged_clear(
    ecs_paged_t *paged)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);

    ecs_size_t page_size = PAGE_SIZE(paged);
    ecs_page_t *pages = _ecs_vector_first(paged->pages, page_size);
    int32_t i, count = ecs_vector_count(paged->pages);

    for (i = 0; i < count; i ++) {
        page_free(paged, ECS_OFFSET(pages, page_size * i));
    }

    ecs_vector_free(paged->pages);

    paged->pages = NULL;
}

void ecs_paged_deinit(
    ecs_paged_t *paged)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);
    ecs_paged_clear(paged);
    ecs_os_free(paged->column_sizes);
    paged->column_sizes  = NULL;
}

void ecs_paged_free(
    ecs_paged_t *paged)
{
    ecs_paged_deinit(paged);
    ecs_os_free(paged);
}

void* _ecs_paged_get(
    const ecs_paged_t *paged,
    ecs_size_t size,
    int32_t index,
    int32_t column)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(column < paged->column_count, ECS_INVALID_PARAMETER);
    ecs_assert(size == paged->column_sizes[column], ECS_INVALID_PARAMETER);
    ecs_page_t *p = page_get(paged, PAGE(index));
    if (p) {
        return ECS_OFFSET(p[column], size * OFFSET(index));
    }
    return NULL;
}

void* _ecs_paged_ensure(
    ecs_paged_t *paged,
    ecs_size_t size,
    int32_t index,
    int32_t column)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(column < paged->column_count, ECS_INVALID_PARAMETER);
    ecs_assert(size == paged->column_sizes[column], ECS_INVALID_PARAMETER);
    ecs_page_t *p = page_ensure(paged, PAGE(index));
    ecs_assert(p != NULL, ECS_INTERNAL_ERROR);
    return ECS_OFFSET(p[column], size * OFFSET(index));
}

int32_t ecs_paged_add(
    ecs_paged_t *paged)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);
    int32_t count = paged->count ++;
    page_ensure(paged, PAGE(count));
    return count;
}

void ecs_paged_remove(
    ecs_paged_t *paged,
    int32_t index)
{
    int32_t i, count = -- paged->count;
    if (count > 1) {
        ecs_page_t *page_dst = page_get(paged, PAGE(index));
        ecs_page_t *page_src = page_get(paged, PAGE(count));

        for (i = 0; i < paged->column_count; i ++) {
            ecs_size_t size = paged->column_sizes[i];
            void *dst = ECS_OFFSET(page_dst[i], size * OFFSET(index));
            void *src = ECS_OFFSET(page_src[i], size * OFFSET(count));
            ecs_os_memcpy(dst, src, size);
        }
    }
}

ecs_page_t* ecs_paged_get_page(
    const ecs_paged_t *paged,
    int32_t index)
{
    return page_get(paged, PAGE(index));
}

ecs_page_t* ecs_paged_ensure_page(
    ecs_paged_t *paged,
    int32_t index)
{
    return page_ensure(paged, PAGE(index));
}

void* _ecs_paged_page_get(
    const ecs_paged_t *paged,
    ecs_page_t *page, 
    ecs_size_t size,
    int32_t index,
    int32_t column)
{
    ecs_assert(paged != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(page != NULL, ECS_INVALID_PARAMETER);
    ecs_assert(size == paged->column_sizes[column], ECS_INVALID_PARAMETER);
    return ECS_OFFSET(page[column], size * OFFSET(index));
}

int32_t ecs_paged_count(
    const ecs_paged_t *paged)
{
    return paged->count;
}
