#pragma once
// The math half of tools/lunar_sky.py in plain C++ (no Unreal, no SPICE). Shared by the LHScience
// module and tools/cpp_check, so the port is tested against the Python oracle without the engine.
// Keep it line for line with lunar_sky.py.
#include <cmath>

namespace LHCore
{
constexpr double RMoonKm = 1737.4;
constexpr double SolarConst = 1361.0;      // W/m^2 at 1 AU (IAU 2015 B3 nominal)
constexpr double AuKm = 149597870.7;
constexpr double SunRadiusKm = 695700.0;
constexpr double EarthRadiusKm = 6371.0;
constexpr double DsnMinElDeg = 10.0;
constexpr double SunLux = 131000.0;        // extraterrestrial solar illuminance (docs/05 section 3.1)
constexpr double Pi = 3.14159265358979323846;

// Loaded in this order from Content/SPICE (docs/02 section 3)
inline constexpr const char* Kernels[] = {
    "naif0012.tls", "pck00011.tpc", "moon_pa_de440_200625.bpc", "moon_de440_250416.tf",
    "de440s.bsp", "earthstns_itrf93_260814.bsp", "earth_topo_260814.tf", "earth_latest_high_prec.bpc"};

inline double Rad(double D) { return D * Pi / 180.0; }
inline double Deg(double R) { return R * 180.0 / Pi; }

// Body-fixed MOON_ME position of a surface site (sphere, R = 1737.4 km)
inline void SiteVectorKm(double LatDeg, double LonDeg, double HeightM, double Out[3])
{
    const double R = RMoonKm + HeightM / 1000.0, La = Rad(LatDeg), Lo = Rad(LonDeg);
    Out[0] = R * std::cos(La) * std::cos(Lo);
    Out[1] = R * std::cos(La) * std::sin(Lo);
    Out[2] = R * std::sin(La);
}

// Fraction of a uniform disk above a (locally flat) horizon line, 0..1
inline double DiskVisibleFraction(double CenterElDeg, double HorizonElDeg, double RadiusDeg)
{
    const double H = (CenterElDeg - HorizonElDeg) / RadiusDeg;   // centre height in disk radii
    if (H >= 1.0) return 1.0;
    if (H <= -1.0) return 0.0;
    return 1.0 - (std::acos(H) - H * std::sqrt(1.0 - H * H)) / Pi;  // minus the circular segment below
}

// Horizon elevation at an azimuth, from N equal bins starting at 0 deg (no mask = flat, 0 deg)
inline double MaskAt(const float* El, int N, double AzDeg)
{
    if (!El || N <= 0) return 0.0;
    const double X = std::fmod(std::fmod(AzDeg, 360.0) + 360.0, 360.0) / (360.0 / N);
    const int I = int(X) % N;
    const double F = X - std::floor(X);
    return El[I] * (1 - F) + El[(I + 1) % N] * F;
}

// Reference panel: vertical, tracking the Sun in azimuth, so the incidence factor is cos(el). Watts.
inline double PanelPowerW(double SunKm, double SunFrac, double SunElDeg, double AreaM2 = 1.0, double Eff = 0.30)
{
    const double Flux = SolarConst * (AuKm / SunKm) * (AuKm / SunKm);
    return Flux * AreaM2 * Eff * SunFrac * std::cos(Rad(SunElDeg));
}

// Earthshine on the surface (lux): Lambert-sphere Earth, geometric albedo 0.367 (docs/05 section 3.2).
// PhaseAngleRad is the Sun-Earth-Moon angle, 0 = full Earth.
inline double EarthshineLux(double PhaseAngleRad, double EarthKm)
{
    const double Full = 0.367 * SunLux * (EarthRadiusKm / EarthKm) * (EarthRadiusKm / EarthKm);
    return Full * (std::sin(PhaseAngleRad) + (Pi - PhaseAngleRad) * std::cos(PhaseAngleRad)) / Pi;
}

// Window metrics, accumulated step by step exactly like lunar_sky.metrics()
struct FMetricsAccum
{
    double StepMin = 30.0;
    int N = 0, Dte = 0, Dsn = 0, Run = 0, Longest = 0;
    double SumLit = 0.0, SumPower = 0.0;
    int NextSunrise = -1, NextEarthrise = -1;   // first step where >= 50 % of the Sun / Earth's centre comes into view
    bool PrevSunlit = false, PrevEarth = false;

    void Add(double SunFrac, bool EarthVis, bool DsnSees, double PowerW)
    {
        const bool Sunlit = SunFrac >= 0.5;
        if (N > 0 && NextSunrise < 0 && Sunlit && !PrevSunlit) NextSunrise = N;
        if (N > 0 && NextEarthrise < 0 && EarthVis && !PrevEarth) NextEarthrise = N;
        PrevSunlit = Sunlit;
        PrevEarth = EarthVis;
        Run = SunFrac < 0.01 ? Run + 1 : 0;
        Longest = Run > Longest ? Run : Longest;
        SumLit += SunFrac;
        SumPower += PowerW;
        Dsn += DsnSees ? 1 : 0;
        Dte += (EarthVis && DsnSees) ? 1 : 0;
        ++N;
    }
    double IlluminationPct() const { return N ? 100.0 * SumLit / N : 0.0; }
    double LongestDarkHours() const { return Longest * StepMin / 60.0; }
    double DTEPct() const { return N ? 100.0 * Dte / N : 0.0; }
    double DSNCoveragePct() const { return N ? 100.0 * Dsn / N : 0.0; }
    double AvgPowerW() const { return N ? SumPower / N : 0.0; }
};
}
