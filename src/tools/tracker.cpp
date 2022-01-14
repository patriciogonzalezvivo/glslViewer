#include "tracker.h"

#include "ada/tools/text.h"

void Tracker::start() {
    m_data.clear();

    auto start = std::chrono::high_resolution_clock::now();
    m_trackerStart = std::chrono::time_point_cast<std::chrono::microseconds>(start).time_since_epoch().count() * 0.001;
    m_running = true;
}

void Tracker::begin(const std::string& _track) {
    if (!m_running)
        return;

    if ( m_data.find(_track) == m_data.end() )
        m_tracks.push_back(_track);

    m_data[_track].start = std::chrono::high_resolution_clock::now();
}

void Tracker::end(const std::string& _track) {
    if (!m_running)
        return;

    auto sample_end = std::chrono::high_resolution_clock::now();

    if ( m_data.find(_track) == m_data.end() )
        m_tracks.push_back(_track);

    auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_data[_track].start).time_since_epoch();
    auto end = std::chrono::time_point_cast<std::chrono::microseconds>(sample_end).time_since_epoch();
    
    StatSample stat;
    stat.startMs = start.count() * 0.001 - m_trackerStart;
    stat.endMs = end.count() * 0.001 - m_trackerStart;
    stat.durationMs = stat.endMs - stat.startMs;

    m_data[_track].samples.push_back( stat );
}

void Tracker::stop() {
    m_running = false;
}

double  Tracker::getFramerate() {
    double frm = 0.0;
    int count = 0;
    for (std::map<std::string, StatTrack>::iterator it = m_data.begin() ; it != m_data.end(); ++it) {   
        double delta = 0.0;
        for (size_t i = 1; i < it->second.samples.size(); i++) 
            delta += (it->second.samples[i].startMs - it->second.samples[i-1].startMs);
        
        delta /= (double) (it->second.samples.size() - 1);
        frm += delta;
        count++;
    }

    return ( frm / (double)count );
}

std::string Tracker::getStack() const {
    std::string stack = "";

    for (size_t i = 0; i < m_stack.size(); i++) {
        if (i > 0)
            stack += ":";
        stack += m_stack[i];
    }

    return stack;
}

std::string Tracker::logFramerate() {
    return  "framerate," + ada::toString(getFramerate()) + ",100.0%\n";// +
            // "fps," + ada::toString( (1./getFramerate()) * 1000.0 ) ;
}

std::string Tracker::logSamples() {
    std::string log = "";

    for (size_t t = 0; t < m_tracks.size(); t++)
        log += logSamples(m_tracks[t]);

    return log;
}

std::string Tracker::logSamples(const std::string& _track) {
    std::map<std::string, StatTrack>::iterator it = m_data.find(_track);

    if ( it == m_data.end() )
        return "";

    std::string log = "";
    std::string track_name = it->first;
    
    for (size_t i = 0; i < it->second.samples.size(); i++)
        log +=  track_name + "," + 
                ada::toString(it->second.samples[i].startMs) + "," + 
                ada::toString(it->second.samples[i].durationMs) + "\n";

    return log;
}

std::string Tracker::logAverage() {
    std::string log = "";

    for (size_t t = 0; t < m_tracks.size(); t++)
        log += logAverage(m_tracks[t]);

    return log;
}

std::string Tracker::logAverage(const std::string& _track) {
    std::map<std::string, StatTrack>::iterator it = m_data.find(_track);

    if ( it == m_data.end() )
        return "";

    std::string log = "";
    std::string track_name = it->first;

    double average = 0.0;
    double delta = 0.0;
    for (size_t i = 0; i < it->second.samples.size(); i++) {
        average += it->second.samples[i].durationMs;
        if (i > 0)
            delta += it->second.samples[i].startMs - it->second.samples[i-1].startMs;
    }

    average /= (double)it->second.samples.size();
    delta /= (double)it->second.samples.size() - 1.0;
    it->second.durationAverage = average;
    
    log += track_name + "," + ada::toString(average) + "," + ada::toString( (average/delta) * 100.0) + "%," + ada::toString(delta) +  "\n";

    return log;
}
