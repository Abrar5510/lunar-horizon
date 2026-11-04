#include "LHSpice.h"
#include "LHCore.h"
#include <cmath>
#include <mutex>

#ifdef LH_STANDALONE
#include "SpiceUsr.h"
#else
#include "HAL/Platform.h"
THIRD_PARTY_INCLUDES_START
#include "SpiceUsr.h"
THIRD_PARTY_INCLUDES_END
#endif

namespace
{
std::recursive_mutex Lock;
std::string LastErr;
const char* const Dsn[] = {"DSS-14", "DSS-43", "DSS-63"};

bool Failed()   // caller holds Lock
{
    if (!failed_c()) return false;
    SpiceChar Msg[1841];
    getmsg_c("LONG", sizeof(Msg), Msg);
    LastErr = Msg;
    reset_c();
    return true;
}
}

bool FLHSpice::Init(const std::vector<std::string>& KernelPaths, std::string& OutError)
{
    std::lock_guard<std::recursive_mutex> Guard(Lock);
    SpiceChar Return[] = "RETURN", None[] = "NONE";
    erract_c("SET", 0, Return);   // never abort() the host process on a SPICE error
    errprt_c("SET", 0, None);     // we read the messages ourselves
    kclear_c();                   // PIE restarts call Init again
    for (const std::string& P : KernelPaths)
    {
        furnsh_c(P.c_str());
        if (Failed()) { OutError = LastErr; return false; }
    }
    return true;
}

bool FLHSpice::UtcToEt(const char* IsoUtc, double& OutEt)
{
    std::lock_guard<std::recursive_mutex> Guard(Lock);
    str2et_c(IsoUtc, &OutEt);
    return !Failed();
}

bool FLHSpice::AzEl(const char* Target, double Et, const double ObsPos[3], const char* ObsCenter,
                    const char* ObsFrame, FLHAzEl& Out)
{
    std::lock_guard<std::recursive_mutex> Guard(Lock);
    SpiceDouble Azl[6], Lt;
    // azccw = false: clockwise from north; elplsz = true: elevation positive up
    azlcpo_c("ELLIPSOID", Target, Et, "LT+S", SPICEFALSE, SPICETRUE, ObsPos, ObsCenter, ObsFrame, Azl, &Lt);
    if (Failed()) return false;
    Out.RangeKm = Azl[0];
    Out.AzDeg = LHCore::Deg(Azl[1]);
    Out.ElDeg = LHCore::Deg(Azl[2]);
    return true;
}

bool FLHSpice::DirMoonMe(const char* Target, double Et, const double ObsPos[3], double OutDir[3])
{
    std::lock_guard<std::recursive_mutex> Guard(Lock);
    SpiceDouble Obs[6] = {ObsPos[0], ObsPos[1], ObsPos[2], 0, 0, 0}, State[6], Lt;   // spkcpo wants a state
    spkcpo_c(Target, Et, "MOON_ME", "OBSERVER", "LT+S", Obs, "MOON", "MOON_ME", State, &Lt);
    if (Failed()) return false;
    vhat_c(State, OutDir);
    return true;
}

bool FLHSpice::AnyDsnSeesMoon(double Et)
{
    std::lock_guard<std::recursive_mutex> Guard(Lock);
    for (const char* Station : Dsn)
    {
        SpiceDouble Pos[3], Lt;
        spkpos_c(Station, Et, "ITRF93", "NONE", "EARTH", Pos, &Lt);
        if (Failed()) return false;
        FLHAzEl Moon;
        if (AzEl("MOON", Et, Pos, "EARTH", "ITRF93", Moon) && Moon.ElDeg >= LHCore::DsnMinElDeg) return true;
    }
    return false;
}

double FLHSpice::EarthPhaseAngleRad(double Et)
{
    std::lock_guard<std::recursive_mutex> Guard(Lock);
    const double A = phaseq_c(Et, "EARTH", "SUN", "MOON", "LT+S");
    return Failed() ? 0.0 : A;
}

bool FLHSpice::Rotation(const char* From, const char* To, double Et, double OutR[3][3])
{
    std::lock_guard<std::recursive_mutex> Guard(Lock);
    pxform_c(From, To, Et, OutR);
    return !Failed();
}

bool FLHSpice::Sky(double Et, const double ObsPos[3], const float* Mask, int MaskNum, FLHSky& Out)
{
    if (!AzEl("SUN", Et, ObsPos, "MOON", "MOON_ME", Out.Sun) || !AzEl("EARTH", Et, ObsPos, "MOON", "MOON_ME", Out.Earth))
        return false;
    const double SunR = LHCore::Deg(std::asin(LHCore::SunRadiusKm / Out.Sun.RangeKm));
    Out.SunFrac = LHCore::DiskVisibleFraction(Out.Sun.ElDeg, LHCore::MaskAt(Mask, MaskNum, Out.Sun.AzDeg), SunR);
    Out.EarthVis = Out.Earth.ElDeg > LHCore::MaskAt(Mask, MaskNum, Out.Earth.AzDeg);
    return true;
}

std::string FLHSpice::LastError()
{
    std::lock_guard<std::recursive_mutex> Guard(Lock);
    return LastErr;
}
