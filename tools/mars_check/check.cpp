// Builds MarsCore.h without Unreal: checks the MOLA grids and the route planner, then prints the
// Mars24 sun for each Unix time given so run.sh can compare it with SPICE.
//   check <data_dir> [unix_seconds ...]
#include "MarsCore.h"
#include <cstdio>
#include <cstdlib>
#include <string>

#define CHECK(X) do { if (!(X)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #X); return 1; } } while (0)

int main(int Argc, char** Argv)
{
    if (Argc < 2) { std::fprintf(stderr, "usage: check data_dir [unix ...]\n"); return 2; }
    const std::string Dir = Argv[1];
    MarsCore::FGrid Hi, Lo;
    CHECK(Hi.Load((Dir + "/megt90n000eb.img").c_str(), 5760));
    CHECK(Lo.Load((Dir + "/megt90n000cb.img").c_str(), 1440));
    CHECK(Hi.Sample(18.65, 226.2) > 20000);    // Olympus Mons summit, ~21.2 km
    CHECK(Hi.Sample(-42.4, 70.5) < -6000);     // Hellas basin floor
    CHECK(std::abs(Hi.Sample(10, 359.99) - Hi.Sample(10, 0.01)) < 200);   // no seam at 0/360

    // Curiosity (Gale) -> Perseverance (Jezero)
    const MarsCore::FRoute R = MarsCore::PlanRoute(Lo, -4.59, 137.44, 18.44, 77.45, 8.0);
    const double Gc = MarsCore::GreatCircleM(-4.59, 137.44, 18.44, 77.45);
    CHECK(R.Ok);
    CHECK(R.DistM > 0.98 * Gc && R.DistM < 1.3 * Gc);
    CHECK(R.MaxSlopeDeg <= 8.0);
    std::fprintf(stderr, "route Gale->Jezero: %.0f km (great circle %.0f km), +%.0f/-%.0f m, max slope %.2f deg, %zu cells\n",
                 R.DistM / 1e3, Gc / 1e3, R.AscentM, R.DescentM, R.MaxSlopeDeg, R.Pts.size());
    // A limit no cell pair meets can't be routed.
    CHECK(!MarsCore::PlanRoute(Lo, 18.65, 226.2, 18.0, 226.2, 0.001).Ok);

    std::printf("unix,ls,sublon,sublat,dist_au\n");
    for (int I = 2; I < Argc; ++I)
    {
        const double T = std::atof(Argv[I]);
        const MarsCore::FSun S = MarsCore::SunAt(T);
        std::printf("%.0f,%.6f,%.6f,%.6f,%.8f\n", T, S.LsDeg, S.SubLonE, S.SubLat, S.DistAu);
    }
    return 0;
}
