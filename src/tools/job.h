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
    Job (std::string file_name, int width, int height, std::unique_ptr<unsigned char[]>&& pixels,
         std::atomic<int>& task_count, std::atomic<long long>& max_mem_in_queue):

        m_file_name(std::move(file_name)),
        m_width(width),
        m_height(height),
        m_pixels(std::move(pixels)),
        m_task_count(&task_count),
        m_max_mem_in_queue(&max_mem_in_queue) {
        if (m_pixels) {
            task_count++;
            max_mem_in_queue -= mem_consumed_by_pixels();
        }
    }

    /** the function that is being invoked when the task is done **/
    void operator()() {
        if (m_pixels) {
            ada::savePixels(m_file_name, m_pixels.get(), m_width, m_height);
            m_pixels = nullptr;
            (*m_task_count)--;
            (*m_max_mem_in_queue) += mem_consumed_by_pixels();
        }
    }
protected:
    long mem_consumed_by_pixels() const { return m_width * m_height * 4; }

    std::string                         m_file_name;
    int                                 m_width;
    int                                 m_height;
    std::unique_ptr<unsigned char[]>    m_pixels;
    std::atomic<int> *                  m_task_count;
    std::atomic<long long> *            m_max_mem_in_queue;

};