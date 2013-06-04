#include "context.hh"
#include "lithe.h"
#include "internal/assert.h"

using namespace lithe;

Context::Context()
{
  stack.size = 0;
  stack.bottom = NULL;
  start_routine = NULL;
  arg = NULL;
}

Context::Context(size_t stack_size, void (*start_routine)(void*), void *arg)
{
  stack.bottom = NULL;
  init(stack_size, start_routine, arg);
  lithe_context_init(this, start_routine_wrapper, this);
}

Context::~Context()
{
  lithe_context_cleanup(this);
  free(stack.bottom);
}

void Context::reinit(size_t stack_size, void (*start_routine)(void*), void *arg)
{
  init(stack_size, start_routine, arg);
  lithe_context_reinit(this, &start_routine_wrapper, this);
}

void Context::start_routine_wrapper(void *__arg)
{
  Context *self = (Context*)__arg;

  self->start_routine(self->arg);
  destroy_dtls();
}

void Context::init(size_t stack_size, void (*start_routine)(void*), void *arg)
{
  this->stack.size = stack_size;
  this->stack.bottom = realloc(this->stack.bottom, stack_size);
  assert(this->stack.bottom);

  this->start_routine = start_routine;
  this->arg = arg;
}
