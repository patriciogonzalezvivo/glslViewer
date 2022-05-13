#pragma once

#include <memory>
#include <atomic>
#include <string>
#include <utility>

#include "ada/tools/pixels.h"

/** Just a small helper that captures all the relevant data to save an image **/
class Job {
public:
    Job (const Job& ) = delete;
    Job (Job && ) = default;
    Job (std::string _filename, int _width, int _height, std::unique_ptr<unsigned char[]>&& _pixels,
         std::atomic<int>& _task_count, std::atomic<long long>& _max_mem_in_queue):

        m_filename(std::move(_filename)),
        m_width(_width),
        m_height(_height),
        m_pixels(std::move(_pixels)),
        m_task_count(&_task_count),
        m_max_mem_in_queue(&_max_mem_in_queue) {
        if (m_pixels) {
            _task_count++;
            _max_mem_in_queue -= mem_consumed_by_pixels();
        }
    }

    /** the function that is being invoked when the task is done **/
    void operator()() {
        if (m_pixels) {
            ada::savePixels(m_filename, m_pixels.get(), m_width, m_height);
            m_pixels = nullptr;
            (*m_task_count)--;
            (*m_max_mem_in_queue) += mem_consumed_by_pixels();
        }
    }
protected:
    long mem_consumed_by_pixels() const { return m_width * m_height * 4; }

    std::string                         m_filename;
    int                                 m_width;
    int                                 m_height;
    std::unique_ptr<unsigned char[]>    m_pixels;
    std::atomic<int> *                  m_task_count;
    std::atomic<long long> *            m_max_mem_in_queue;

};