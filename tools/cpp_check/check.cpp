// Builds the LHScience core (LHCore.h + LHSpice.cpp) without Unreal and checks it row by row
// against a tools/lunar_sky.py --csv time series. Run tools/cpp_check/run.sh.
//   check <kernel_dir> <oracle.csv> <lat> <lon> <height_m> [mask.csv]
#include "LHCore.h"
#include "LHSpice.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

static std::vector<std::string> Split(const std::string& Line)
{
    std::vector<std::string> Out;
    std::stringstream Ss(Line.substr(0, Line.find('\r')));   // Python's csv writes \r\n
    for (std::string C; std::getline(Ss, C, ',');) Out.push_back(C);
    return Out;
}

int main(int Argc, char** Argv)
{
    if (Argc < 6) { std::fprintf(stderr, "usage: check kernels oracle.csv lat lon height_m [mask.csv]\n"); return 2; }
    std::vector<std::string> Paths;
    for (const char* K : LHCore::Kernels) Paths.push_back(std::string(Argv[1]) + "/" + K);
    std::string Err;
    if (!FLHSpice::Init(Paths, Err)) { std::fprintf(stderr, "SPICE: %s\n", Err.c_str()); return 1; }

    std::vector<float> Mask;
    if (Argc > 6)
    {
        std::ifstream M(Argv[6]);
        std::string Line;
        std::getline(M, Line);   // header
        while (std::getline(M, Line)) Mask.push_back(std::stof(Split(Line).at(1)));
    }
    double Obs[3];
    LHCore::SiteVectorKm(std::atof(Argv[3]), std::atof(Argv[4]), std::atof(Argv[5]), Obs);

    std::ifstream In(Argv[2]);
    std::string Line;
    std::getline(In, Line);
    std::map<std::string, size_t> Col;
    for (const std::string& H : Split(Line)) Col[H] = Col.size();

    LHCore::FMetricsAccum Acc;
    double WorstAng = 0, WorstFrac = 0, WorstPow = 0;
    int Rows = 0, Mismatch = 0;
    while (std::getline(In, Line))
    {
        const std::vector<std::string> C = Split(Line);
        auto Num = [&](const char* K) { return std::stod(C.at(Col.at(K))); };
        auto Flag = [&](const char* K) { return C.at(Col.at(K)) == "True"; };
        double Et;
        FLHSky S;
        if (!FLHSpice::UtcToEt(C.at(0).c_str(), Et) || !FLHSpice::Sky(Et, Obs, Mask.data(), int(Mask.size()), S))
        {
            std::fprintf(stderr, "SPICE: %s\n", FLHSpice::LastError().c_str());
            return 1;
        }
        const bool Dsn = FLHSpice::AnyDsnSeesMoon(Et);
        const double P = LHCore::PanelPowerW(S.Sun.RangeKm, S.SunFrac, S.Sun.ElDeg);
        Acc.Add(S.SunFrac, S.EarthVis, Dsn, P);

        auto DAz = [](double A, double B) { return std::fabs(std::remainder(A - B, 360.0)); };
        const double Ang = std::fmax(std::fmax(std::fabs(S.Sun.ElDeg - Num("sun_el")), std::fabs(S.Earth.ElDeg - Num("earth_el"))),
                                     std::fmax(DAz(S.Sun.AzDeg, Num("sun_az")), DAz(S.Earth.AzDeg, Num("earth_az"))));
        WorstAng = std::fmax(WorstAng, Ang);
        WorstFrac = std::fmax(WorstFrac, std::fabs(S.SunFrac - Num("sun_frac")));
        WorstPow = std::fmax(WorstPow, std::fabs(P - Num("power_w")));
        Mismatch += (S.EarthVis != Flag("earth_vis")) + (Dsn != Flag("dsn"));
        ++Rows;
    }
    // CSV values are rounded to 5 decimals (power to 0.01 W)
    const bool Ok = Rows > 0 && WorstAng < 1e-4 && WorstFrac < 1e-4 && WorstPow < 0.011 && Mismatch == 0;
    std::printf("rows=%d worst_angle=%.2e deg worst_sun_frac=%.2e worst_power=%.3f W flag_mismatches=%d\n",
                Rows, WorstAng, WorstFrac, WorstPow, Mismatch);
    std::printf("{\"IlluminationPct\": %.4f, \"LongestDarkHours\": %.1f, \"DTEPct\": %.4f, \"DSNCoveragePct\": %.4f, \"AvgPowerW\": %.4f}\n",
                Acc.IlluminationPct(), Acc.LongestDarkHours(), Acc.DTEPct(), Acc.DSNCoveragePct(), Acc.AvgPowerW());
    std::printf("%s\n", Ok ? "C++ matches Python oracle" : "MISMATCH");
    return Ok ? 0 : 1;
}
