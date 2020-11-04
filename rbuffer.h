#ifndef RBUFFER_H
#define RBUFFER_H

#include <mutex>
#include <condition_variable>
#include <cstddef>

template <class T> class rbuffer {
    public:
        rbuffer(size_t capacity = 100);
        ~rbuffer();

        void close();
        bool is_empty();
        bool is_open();
        void finish();
        bool add(T * elem);
        bool remove(T * elem);

    private:
        std::mutex * cv_mtx;
        std::atomic_bool sopen;
        std::condition_variable empty_cv;
        std::condition_variable full_cv;
        std::condition_variable fin_cv;

        size_t start;
        std::atomic_size_t end;
        std::atomic_size_t cap;
        std::atomic_size_t len;

        std::mutex * buffer_mtx;
        T * buffer; 
};

template <class T> rbuffer<T>::rbuffer(size_t capacity) {
    this->cap = capacity;
    this->start = 0;
    this->end = 0;
    this->len = 0;

    this->sopen = true;

    this->buffer = new T[capacity];
    this->buffer_mtx = new std::mutex();
    this->cv_mtx = new std::mutex();
}

template <class T> rbuffer<T>::~rbuffer() {
    delete this->buffer;
    delete this->buffer_mtx;
    delete this->cv_mtx;
}


template <class T> void rbuffer<T>::close() {
    std::lock_guard<std::mutex> lk(*this->cv_mtx); 
    this->sopen = false;

    //Notify all threads, they should all exit their methods returning false
    empty_cv.notify_all();
    full_cv.notify_all();
    
    fin_cv.notify_all();
}

template <class T> bool rbuffer<T>::is_empty(){
    return this->start == this->end;
}

template <class T> bool rbuffer<T>::is_open(){
    return this->sopen;
}

template <class T> void rbuffer<T>::finish(){ 
    this->close();

    std::unique_lock<std::mutex> lk(*this->cv_mtx);
    this->fin_cv.wait(lk, [this]{  return this->len == 0; });
}

template<class T> bool rbuffer<T>::add(T * elem) {
    //if the stream is closed then just return
    if(!this->sopen) {
        return false;
    }
    
    //wait until buffer has an opening
    std::unique_lock<std::mutex> lk(*this->cv_mtx);
    this->full_cv.wait(lk, [this] { return ((this->len != this->cap - 1) && this->sopen); });
    
    this->buffer_mtx->lock();
    this->len++;
    
    //insert the value into the buffer
    this->buffer[(this->end++) % this->cap] = *elem;
    //notify any threads that may be waiting on an element to appear
    this->empty_cv.notify_one();
    this->buffer_mtx->unlock();
    return true;
}

template<class T> bool rbuffer<T>::remove(T * elem) {
    //wait until buffer has an element
    std::unique_lock<std::mutex> lk(*this->cv_mtx);
    this->empty_cv.wait(lk, [this] {  return this->len != 0 || !this->sopen; });
    //if the stream is closed then just return
    if(!this->sopen && (this->len == 0)) {
        return false;
    }

    this->buffer_mtx->lock();
    this->len --;

    //get the element
    *elem = this->buffer[(this->start++) % this->cap];
    //notify any threads waiting for an elememt to be removed
    this->full_cv.notify_one(); 
    this->fin_cv.notify_all();
    this->buffer_mtx->unlock();
    return true;
}

#endif
