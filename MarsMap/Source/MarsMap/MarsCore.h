#pragma once
// Mars science core: plain C++ (no Unreal) so tools/mars_check can build and test it with clang.
// Longitudes are planetocentric degrees EAST (0..360), the MOLA MEGDR convention.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <queue>
#include <vector>

namespace MarsCore
{
constexpr double Pi = 3.14159265358979323846;
constexpr double D2R = Pi / 180.0, R2D = 180.0 / Pi;
constexpr double RadiusM = 3396000.0;     // MOLA MEGDR reference sphere (label A_AXIS_RADIUS)
constexpr double TtMinusUtc = 69.184;     // 37 leap s + 32.184 s; valid from 2017-01-01 until the next leap second

inline double Wrap(double V, double M) { V = std::fmod(V, M); return V < 0 ? V + M : V; }

inline void Unit(double LatDeg, double LonEDeg, double Out[3])
{
    const double La = LatDeg * D2R, Lo = LonEDeg * D2R;
    Out[0] = std::cos(La) * std::cos(Lo);
    Out[1] = -std::cos(La) * std::sin(Lo);   // Unreal is left-handed: east must be -Y to read correctly from outside
    Out[2] = std::sin(La);
}

inline double GreatCircleM(double Lat0, double Lon0, double Lat1, double Lon1)
{
    const double A = std::sin((Lat1 - Lat0) * D2R / 2), B = std::sin((Lon1 - Lon0) * D2R / 2);
    const double H = A * A + std::cos(Lat0 * D2R) * std::cos(Lat1 * D2R) * B * B;
    return 2 * RadiusM * std::asin(std::sqrt(std::min(1.0, H)));
}

// ---- Sun and Mars time: Mars24 (Allison & McEwen 2000; giss.nasa.gov/tools/mars24/help/algorithm.html) ----
struct FSun
{
    double Msd, MtcHours, LsDeg, EotDeg, DistAu;
    double SubLonE, SubLat;   // subsolar point (planetographic latitude)
};

inline FSun SunAt(double UnixSeconds)
{
    const double Dt = 2440587.5 + (UnixSeconds + TtMinusUtc) / 86400.0 - 2451545.0;   // days since J2000 TT
    const double M = (19.3871 + 0.52402073 * Dt) * D2R;
    const double Afms = 270.3871 + 0.524038496 * Dt;
    static const double A[7] = {0.0071, 0.0057, 0.0039, 0.0037, 0.0021, 0.0020, 0.0018};
    static const double Tau[7] = {2.2353, 2.7543, 1.1177, 15.7866, 2.1354, 2.4694, 32.8493};
    static const double Phi[7] = {49.409, 168.173, 191.837, 21.736, 15.704, 95.528, 49.095};
    double Pbs = 0;
    for (int I = 0; I < 7; ++I) Pbs += A[I] * std::cos((0.985626 * Dt / Tau[I] + Phi[I]) * D2R);
    const double NuM = (10.691 + 3.0e-7 * Dt) * std::sin(M) + 0.623 * std::sin(2 * M) + 0.050 * std::sin(3 * M)
                     + 0.005 * std::sin(4 * M) + 0.0005 * std::sin(5 * M) + Pbs;
    FSun S;
    S.LsDeg = Wrap(Afms + NuM, 360.0);
    const double Ls = S.LsDeg * D2R;
    S.EotDeg = 2.861 * std::sin(2 * Ls) - 0.071 * std::sin(4 * Ls) + 0.002 * std::sin(6 * Ls) - NuM;
    S.Msd = (Dt - 4.5) / 1.0274912517 + 44796.0 - 0.0009626;
    S.MtcHours = Wrap(24.0 * S.Msd, 24.0);
    S.DistAu = 1.523679 * (1.00436 - 0.09309 * std::cos(M) - 0.004336 * std::cos(2 * M)
                           - 0.00031 * std::cos(3 * M) - 0.00003 * std::cos(4 * M));
    S.SubLonE = Wrap((12.0 - S.MtcHours) * 15.0 - S.EotDeg, 360.0);   // where local true solar time is noon
    S.SubLat = std::asin(0.42565 * std::sin(Ls)) * R2D + 0.25 * std::sin(Ls);
    return S;
}

inline double LocalSolarHours(const FSun& S, double LonE) { return Wrap(S.MtcHours + (LonE + S.EotDeg) / 15.0, 24.0); }

inline double SunElevationDeg(const FSun& S, double Lat, double LonE)
{
    const double H = (LocalSolarHours(S, LonE) - 12.0) * 15.0 * D2R, La = Lat * D2R, De = S.SubLat * D2R;
    return std::asin(std::sin(La) * std::sin(De) + std::cos(La) * std::cos(De) * std::cos(H)) * R2D;
}

// Mean surface pressure from elevation: 610 Pa at the MOLA datum, 11.1 km scale height (varies ~25% with season).
inline double PressurePa(double ElevM) { return 610.0 * std::exp(-ElevM / 11100.0); }

// ---- MOLA MEGDR elevation grid: big-endian int16 metres; row 0 at 90N, column 0 at 0E ----
struct FGrid
{
    int W = 0, H = 0;
    std::vector<int16_t> M;

    double Ppd() const { return W / 360.0; }

    bool Load(const char* Path, int Width)
    {
        std::ifstream F(Path, std::ios::binary);
        W = Width; H = Width / 2;
        M.resize(size_t(W) * H);
        if (!F.read(reinterpret_cast<char*>(M.data()), std::streamsize(M.size() * 2))) { W = H = 0; M.clear(); return false; }
        for (int16_t& V : M) { const uint16_t U = uint16_t(V); V = int16_t(uint16_t(U << 8 | U >> 8)); }
        return true;
    }

    double At(int X, int Y) const
    {
        X = ((X % W) + W) % W;
        Y = std::clamp(Y, 0, H - 1);
        return M[size_t(Y) * W + X];
    }

    double Sample(double Lat, double LonE) const   // bilinear between cell centres
    {
        const double X = Wrap(LonE, 360.0) * Ppd() - 0.5, Y = (90.0 - Lat) * Ppd() - 0.5;
        const int X0 = int(std::floor(X)), Y0 = int(std::floor(Y));
        const double Fx = X - X0, Fy = Y - Y0;
        const double Top = At(X0, Y0) * (1 - Fx) + At(X0 + 1, Y0) * Fx;
        const double Bot = At(X0, Y0 + 1) * (1 - Fx) + At(X0 + 1, Y0 + 1) * Fx;
        return Top * (1 - Fy) + Bot * Fy;
    }
};

// ---- Traverse planner: A* over grid cells, 8 neighbours, cost = distance x slope penalty ----
struct FRoutePt { double Lat, LonE, ElevM, DistM; };
struct FRoute
{
    bool Ok = false;
    std::vector<FRoutePt> Pts;
    double DistM = 0, AscentM = 0, DescentM = 0, MaxSlopeDeg = 0;
};

// Slopes are regional: measured between cell centres (~15 km apart on the 4 ppd grid), so a real
// rover limit (~30 deg on metre scales) has no meaning here; pass a regional limit of a few degrees.
inline FRoute PlanRoute(const FGrid& G, double Lat0, double Lon0, double Lat1, double Lon1, double MaxSlopeDeg)
{
    FRoute R;
    if (!G.W) return R;
    const int W = G.W, H = G.H;
    const double P = G.Ppd();
    auto LatOf = [&](int Y) { return 90.0 - (Y + 0.5) / P; };
    auto LonOf = [&](int X) { return (X + 0.5) / P; };
    auto CellOf = [&](double La, double Lo) {
        return std::clamp(int((90.0 - La) * P), 0, H - 1) * W + int(Wrap(Lo, 360.0) * P) % W;
    };
    // Step lengths depend only on the row: east-west, row y -> y+1 straight and diagonal.
    std::vector<double> Ew(H), Down(H), Diag(H);
    for (int Y = 0; Y < H; ++Y)
    {
        Ew[Y] = GreatCircleM(LatOf(Y), 0, LatOf(Y), 1 / P);
        Down[Y] = GreatCircleM(LatOf(Y), 0, LatOf(Y + 1), 0);
        Diag[Y] = GreatCircleM(LatOf(Y), 0, LatOf(Y + 1), 1 / P);
    }
    const int Start = CellOf(Lat0, Lon0), Goal = CellOf(Lat1, Lon1);
    const double GoalLat = LatOf(Goal / W), GoalLon = LonOf(Goal % W);
    const double TanMax = std::tan(MaxSlopeDeg * D2R);
    auto Heur = [&](int C) { return GreatCircleM(LatOf(C / W), LonOf(C % W), GoalLat, GoalLon); };

    std::vector<float> Cost(size_t(W) * H, std::numeric_limits<float>::infinity());
    std::vector<int> Parent(size_t(W) * H, -1);
    using FItem = std::pair<double, int>;
    std::priority_queue<FItem, std::vector<FItem>, std::greater<FItem>> Open;
    Cost[Start] = 0;
    Open.push({Heur(Start), Start});
    while (!Open.empty())
    {
        const auto [F, C] = Open.top();
        Open.pop();
        if (C == Goal) { R.Ok = true; break; }
        const double Gc = Cost[C];
        if (F > Gc + Heur(C) + 1.0) continue;   // stale queue entry
        const int X = C % W, Y = C / W;
        const double Hc = G.M[C];
        for (int Dy = -1; Dy <= 1; ++Dy)
        {
            const int Ny = Y + Dy;
            if (Ny < 0 || Ny >= H) continue;
            for (int Dx = -1; Dx <= 1; ++Dx)
            {
                if (!Dx && !Dy) continue;
                const int N = Ny * W + (X + Dx + W) % W;
                const int Row = Dy > 0 ? Y : Ny;   // row the vertical step starts from
                const double D = !Dy ? Ew[Y] : (!Dx ? Down[Row] : Diag[Row]);
                const double Slope = std::abs(G.M[N] - Hc) / D;
                if (Slope > TanMax) continue;
                const double S = Slope / TanMax;
                const double Nc = Gc + D * (1.0 + 2.0 * S * S);   // >= distance, so the heuristic stays admissible
                if (Nc < Cost[N]) { Cost[N] = float(Nc); Parent[N] = C; Open.push({Nc + Heur(N), N}); }
            }
        }
    }
    if (!R.Ok) return R;
    std::vector<int> Cells;
    for (int C = Goal; C != -1; C = Parent[C]) Cells.push_back(C);
    std::reverse(Cells.begin(), Cells.end());
    for (size_t I = 0; I < Cells.size(); ++I)
    {
        const int C = Cells[I];
        FRoutePt Pt{LatOf(C / W), LonOf(C % W), double(G.M[C]), 0};
        if (I)
        {
            const FRoutePt& Prev = R.Pts.back();
            const double D = GreatCircleM(Prev.Lat, Prev.LonE, Pt.Lat, Pt.LonE);
            const double Dh = Pt.ElevM - Prev.ElevM;
            Pt.DistM = Prev.DistM + D;
            (Dh > 0 ? R.AscentM : R.DescentM) += std::abs(Dh);
            R.MaxSlopeDeg = std::max(R.MaxSlopeDeg, std::atan2(std::abs(Dh), D) * R2D);
        }
        R.Pts.push_back(Pt);
    }
    R.DistM = R.Pts.back().DistM;
    return R;
}
} // namespace MarsCore
