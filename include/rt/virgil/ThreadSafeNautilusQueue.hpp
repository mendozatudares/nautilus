/*
 * Copyright 2017 - 2019  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadSafeNautilusQueue class.
 * Provides a wrapper around a basic queue to provide thread safety
 * using Nautilus-specific mechanisms
 */
#pragma once


extern "C" {
#include "nautilus_interface.h"
}

#include <ThreadSafeQueue.hpp>



namespace MARC {

  template <typename T>
  class ThreadSafeNautilusQueue final : public ThreadSafeQueue<T> {
    using Base = MARC::ThreadSafeQueue<T>;

    public:

      /*
       * Default constructor
       */
      ThreadSafeNautilusQueue ();

      /*
       * Attempt to get the first value in the queue.
       * Returns true if a value was successfully written to the out parameter, false otherwise.
       */
      bool tryPop (T& out) override ;

      /*
       * Get the first value in the queue.
       * Will block until a value is available unless clear is called or the instance is destructed.
       * Returns true if a value was successfully written to the out parameter, false otherwise.
       */
      bool waitPop (T& out) override ;
      bool waitPop (void) override ;

      /*
       * Push a new value onto the queue.
       */
      void push (T value) override ;

      /*
       * Push a new value onto the queue if the queue size is less than maxSize.
       * Otherwise, wait for it to happen and then push the new value.
       */
      bool waitPush (T value, int64_t maxSize) override ;

      /*
       * Clear all items from the queue.
       */
      void clear (void) override ;

      /*
       * Check whether or not the queue is empty.
       */
      bool empty (void) const override ;

      /*
       * Return the number of elements in the queue.
       */
      int64_t size (void) const override ;

      /*
       * Destructor.
       */
      ~ThreadSafeNautilusQueue(void);

      /*
       * Not copyable.
       */
      ThreadSafeNautilusQueue (const ThreadSafeNautilusQueue & other) = delete;
      ThreadSafeNautilusQueue & operator= (const ThreadSafeNautilusQueue & other) = delete;

      /*
       * Not assignable.
       */
      ThreadSafeNautilusQueue (const ThreadSafeNautilusQueue && other) = delete;
      ThreadSafeNautilusQueue & operator= (const ThreadSafeNautilusQueue && other) = delete;

    private:
      LOCK_TYPE lock;
  };
}

template <typename T>
MARC::ThreadSafeNautilusQueue<T>::ThreadSafeNautilusQueue(){
  LOCK_INIT(&this->lock);
  return ;
}

template <typename T>
bool MARC::ThreadSafeNautilusQueue<T>::tryPop (T& out){
  LOCK_DECL;
  
  LOCK(&this->lock);
  
  
  
  if(Base::m_queue.empty() || !Base::isValid()) {
    UNLOCK(&this->lock);
    return false;
  }
  
  this->internal_pop(out);
  
  UNLOCK(&this->lock);
  
  return true;
}

template <typename T>
bool MARC::ThreadSafeNautilusQueue<T>::waitPop (T& out){
  LOCK_DECL;

  LOCK(&this->lock);

  /*
   * Check if the queue is not valid anymore.
   */
  if(!Base::isValid()) {
    UNLOCK(&this->lock);
    return false;
  }

  /*
   * Wait until the queue will be in a valid state and it will be not empty.
   */
  while (Base::isValid() && Base::m_queue.empty()){
    UNLOCK(&this->lock);
    LOCK(&this->lock);
  }

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!Base::isValid()) {
    UNLOCK(&this->lock);
    return false;
  }

  this->internal_pop(out);

  UNLOCK(&this->lock);
  return true;
}

template <typename T>
bool MARC::ThreadSafeNautilusQueue<T>::waitPop (void){
  LOCK_DECL;
  
  LOCK(&this->lock);

  /*
   * Check if the queue is not valid anymore.
   */
  if(!Base::isValid()) {
    UNLOCK(&this->lock);
    return false;
  }

  /*
   * Wait until the queue will be in a valid state and it will be not empty.
   */
  while (Base::isValid() && Base::m_queue.empty()){
    UNLOCK(&this->lock);
    LOCK(&this->lock);
  }

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!Base::isValid()) {
    UNLOCK(&this->lock);
    return false;
  }

  /*
   * Pop the top element from the queue.
   */
  this->Base::m_queue.pop();

  UNLOCK(&this->lock);
  return true;
}

template <typename T>
void MARC::ThreadSafeNautilusQueue<T>::push (T value){
  LOCK_DECL;
  
  LOCK(&this->lock);
  
  this->internal_push(value);
  
  UNLOCK(&this->lock);

  return ;
}
 
template <typename T>
bool MARC::ThreadSafeNautilusQueue<T>::waitPush (T value, int64_t maxSize){
  LOCK_DECL;
  
  LOCK(&this->lock);

  while (Base::m_queue.size() >= maxSize && Base::isValid()){
    UNLOCK(&this->lock);
    LOCK(&this->lock);
  }

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!Base::isValid()) {
    UNLOCK(&this->lock);
    return false;
  }

  this->internal_push(value);

  UNLOCK(&this->lock);
  
  return true;
}

template <typename T>
bool MARC::ThreadSafeNautilusQueue<T>::empty (void) const {
  LOCK_DECL;
  
  LOCK((volatile spinlock_t *)(&this->lock));
  
  auto empty = Base::m_queue.empty();
  
  UNLOCK((volatile spinlock_t *)(&this->lock));

  return empty;
}

template <typename T>
int64_t MARC::ThreadSafeNautilusQueue<T>::size (void) const{
  LOCK_DECL;
  
  LOCK((volatile spinlock_t *)(&this->lock));
  
  auto s = Base::m_queue.size();
  
  UNLOCK((volatile spinlock_t *)(&this->lock));

  return s;
}

template <typename T>
void MARC::ThreadSafeNautilusQueue<T>::clear (void) {
  LOCK_DECL;
  
  LOCK(&this->lock);
  
  while(!Base::m_queue.empty()) {
    Base::m_queue.pop();
  }

  UNLOCK(&this->lock);
  
  return ;
}

template <typename T>
MARC::ThreadSafeNautilusQueue<T>::~ThreadSafeNautilusQueue(void){
  this->invalidate();

  return ;
}
