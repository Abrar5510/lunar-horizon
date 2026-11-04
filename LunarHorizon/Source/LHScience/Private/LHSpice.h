#pragma once
// The only CSPICE wrapper. Plain C++ so tools/cpp_check builds it without Unreal.
// CSPICE is not thread-safe: every call goes through one lock.
#include <string>
#include <vector>

struct FLHAzEl { double AzDeg = 0, ElDeg = 0, RangeKm = 0; };

// One instant at one site, mirrors lunar_sky.sky_state()
struct FLHSky
{
    FLHAzEl Sun, Earth;
    double SunFrac = 0;       // horizon-masked share of the solar disk
    bool EarthVis = false;    // Earth's centre above the terrain
};

class FLHSpice
{
public:
    static bool Init(const std::vector<std::string>& KernelPaths, std::string& OutError);
    static bool UtcToEt(const char* IsoUtc, double& OutEt);
    // Apparent (LT+S) azimuth (from north, clockwise) / elevation of Target from a body-fixed point
    static bool AzEl(const char* Target, double Et, const double ObsPos[3], const char* ObsCenter,
                     const char* ObsFrame, FLHAzEl& Out);
    // Unit vector from a lunar site to Target, MOON_ME, apparent (LT+S)
    static bool DirMoonMe(const char* Target, double Et, const double ObsPos[3], double OutDir[3]);
    // Any 70 m DSN antenna (Goldstone, Canberra, Madrid) has the Moon >= 10 deg up
    static bool AnyDsnSeesMoon(double Et);
    // Sun-Earth-Moon angle, 0 = full Earth seen from the Moon
    static double EarthPhaseAngleRad(double Et);
    // R such that v_To = R * v_From
    static bool Rotation(const char* From, const char* To, double Et, double OutR[3][3]);
    // Mask: MaskNum equal azimuth bins from 0 deg, or null for a flat horizon
    static bool Sky(double Et, const double ObsPos[3], const float* Mask, int MaskNum, FLHSky& Out);
    static std::string LastError();
};
