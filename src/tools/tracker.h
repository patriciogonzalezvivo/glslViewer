#pragma once

#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>

typedef std::chrono::time_point<std::chrono::high_resolution_clock> StatPoint;

struct StatSample {
    double       startMs;
    double       endMs;
    double       durationMs;
};

struct StatTrack {
    std::string             stack;
    StatPoint               start;
    std::vector<StatSample> samples;
    double                  durationAverage;
};

class Tracker {
public:

    void    start();
    void    stop();

    void    begin(const std::string& _track);
    void    end(const std::string& _track);

    double  getFramerate();

    std::string logSamples();
    std::string logSamples(const std::string& _track);
    std::string logAverage();
    std::string logAverage(const std::string& _track);
    std::string logFramerate();

    bool    isRunning() const { return m_running; }

protected:

    double                  m_trackerStart;

    std::vector<std::string>            m_tracks;
    std::map<std::string, StatTrack>    m_data;

    bool                    m_running = false;

};