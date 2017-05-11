#ifndef MEM_TOOLS_H
#define MEM_TOOLS_H
#include "mem_intercept.h"

#define  ENABLE_TICKS 1

/* return the address of the instruction that called the current function */
void* get_caller_rip(int depth);

/* return the name (function name +line) of the instruction that called the current function */
char* get_caller_function(int depth);

/* return the name (function name +line) of the instruction located at address rip */
char* get_caller_function_from_rip(void* rip);

void print_backtrace(int backtrace_max_depth);


#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

#define FOREACH_TICK(TICK)			\
  TICK(record_malloc)				\
  TICK(record_free)				\
  TICK(fast_alloc)				\
  TICK(sampling_start)				\
  TICK(collect_samples)

enum tick_ids{
  FOREACH_TICK(GENERATE_ENUM)
  NTICKS
};
static const char *tick_names[] = {
  FOREACH_TICK(GENERATE_STRING)
};

struct tick {
  enum tick_ids id;
  char tick_name[80];
  struct timespec start_tick;
  struct timespec stop_tick;
  unsigned nb_calls;
  double total_duration;
};
extern __thread struct tick tick_array[NTICKS];

#define TIME_DIFF(t1, t2) (((t2).tv_sec-(t1).tv_sec)*1e9+((t2).tv_nsec-(t1).tv_nsec))
#define init_tick(tick_id) do {			\
    struct tick*t = &tick_array[tick_id];	\
    sprintf(t->tick_name, "%s", tick_names[tick_id]);	\
    t->nb_calls=0;				\
    t->total_duration=0;			\
  }while(0)

#ifdef ENABLE_TICKS
#define start_tick(tick_id) do {					\
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tick_array[tick_id].start_tick); \
  } while(0)

#define stop_tick(tick_id) do {					\
    struct tick*t = &tick_array[tick_id];			\
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t->stop_tick);	\
    t->nb_calls++;						\
    t->total_duration+=TIME_DIFF(t->start_tick, t->stop_tick);	\
  } while(0)
#else

#define start_tick(tick_id) do {		\
  } while(0)

#define stop_tick(tick_id) do {			\
    struct tick*t = &tick_array[tick_id];	\
    t->nb_calls++;				\
  } while(0)
#endif

struct mem_allocator {
  struct mem_allocator* next_mem; /* next block of memory */
  void* first_block; /* first available block */
  void* buffer_addr; /* address of the buffer as returned by malloc */
  size_t block_size; /* size of each block */
  unsigned long nb_allocated; /* number of blocks allocated in this buffer */
  unsigned long nb_free; /* number of available blocks */
};

static void mem_allocator_init(struct mem_allocator **mem,
			       size_t block_size,
			       unsigned long nb_blocks) {
  *mem = libmalloc(sizeof(struct mem_allocator));
  (*mem)->next_mem = NULL;
  (*mem)->buffer_addr = libmalloc(block_size * nb_blocks);
  (*mem)->first_block = (*mem)->buffer_addr;
  (*mem)->block_size = block_size;
  (*mem)->nb_allocated = nb_blocks;
  (*mem)->nb_free = nb_blocks;
  int i;
  void**ptr = (*mem)->first_block;
  /* create a linked list of blocks */
  for(i=0; i<nb_blocks-1; i++) {
    *ptr = ((uint8_t*)ptr) + block_size;
    ptr = *ptr;
  }
  *ptr = NULL;
}

static void mem_allocator_finalize(struct mem_allocator *mem) {
  if(mem) {
    mem_allocator_finalize(mem->next_mem);
    libfree(mem->buffer_addr);
    free(mem);
  }
}

static void* mem_allocator_alloc(struct mem_allocator *mem) {
  while(mem->nb_free == 0) {
    /* find a mem block with available blocks */
    if(mem->next_mem == NULL) {
      /* no more blocks in the current mem block, allocate a new one */
      mem_allocator_init(&mem->next_mem, mem->block_size, mem->nb_allocated);
      return mem_allocator_alloc(mem->next_mem);
    }
    mem = mem->next_mem;
  }

  /* return the first available block */
  assert(mem);
  assert(mem->nb_free > 0);
  assert(mem->first_block);
  void* retval = mem->first_block;
  mem->first_block = *(void**)mem->first_block;
  mem->nb_free--;
  return retval;
}

static void mem_allocator_free(struct mem_allocator *mem, void* ptr) {
  assert(mem);
  *(void**)ptr = mem->first_block;
  mem->first_block = ptr;
  mem->nb_free++;
}

#endif
