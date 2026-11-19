#include "../Tower_Triumph/Tower_Triumph/StandardSkillRenderer.h"
namespace Standard=StandardSkillPresentation;
constexpr bool StandardNear(float a,float b){return a>b-.00001f && a<b+.00001f;}
constexpr bool EverySkillCovered()
{
    int standard=0,signature=0;int owners[4]{};
    for(int owner=1;owner<=3;++owner)for(int element=1;element<=5;++element)for(int slot=1;slot<=3;++slot)
    {
        const int id=owner*1000+element*100+slot;
        const auto profile=Standard::ProfileFor(id);
        const bool special=SkillPresentation::Requested(id)!=SkillPresentation::Kind::Normal;
        if(Standard::Valid(profile)==special)return false;
        if(special){++signature;continue;}
        ++standard;++owners[owner];
        if(profile.skillID!=id || static_cast<int>(profile.owner)!=owner || static_cast<int>(profile.element)!=element)return false;
        const int bitmap=Standard::BitmapID(profile);
        if(bitmap!=0 && (!Standard::IsBitmap(bitmap) || SkillPresentation::IsUltimateBitmap(bitmap) ||
            SkillPresentation::IsFlightBitmap(bitmap) || BattleActionTiming::IsProjectileEffect(bitmap) ||
            BattleActionTiming::IsContactEffect(bitmap)))return false;
    }
    return standard==30 && signature==15 && owners[1]==10 && owners[2]==10 && owners[3]==10;
}
static_assert(EverySkillCovered(),"All 45 skills resolve once: thirty explicit standard profiles and fifteen unchanged signature profiles");
static_assert(Standard::MirrorImpact(Standard::ProfileFor(1101),-1) &&
    Standard::MirrorImpact(Standard::ProfileFor(2202),-1) && Standard::MirrorImpact(Standard::ProfileFor(3202),-1) &&
    !Standard::MirrorImpact(Standard::ProfileFor(2202),1) && !Standard::MirrorImpact(Standard::ProfileFor(3202),1) &&
    !Standard::MirrorImpact(Standard::ProfileFor(2502),-1) && !Standard::MirrorImpact(Standard::ProfileFor(3201),-1) &&
    !Standard::MirrorImpact(Standard::ProfileFor(3302),-1),
    "Only Knight strikes and directional Water line impacts mirror with facing; other element and single-target art is unchanged");
static_assert(!Standard::Valid(Standard::ProfileFor(0)) && !Standard::Valid(Standard::ProfileFor(3103)) &&
    Standard::ProfileFor(1303).motion==Standard::Motion::Smash && Standard::ProfileFor(1402).motion==Standard::Motion::Break &&
    Standard::ProfileFor(2302).motion==Standard::Motion::FanVolley && Standard::ProfileFor(3301).motion==Standard::Motion::VineWhip &&
    Standard::ProfileFor(3401).motion==Standard::Motion::GrowingRay && Standard::ProfileFor(3502).motion==Standard::Motion::ShadowSpike,
    "Heavy strike, armor break, fan volley, vine, ray and shadow spike are distinct skills, not number-derived tiers");
static_assert(Standard::ImpactAt(Standard::ProfileFor(1101),.299f,.3f).visible==false &&
    Standard::ImpactAt(Standard::ProfileFor(1101),.3f,.3f).frame==0 &&
    Standard::ImpactAt(Standard::ProfileFor(1103),.3f,.3f).frame==6 &&
    StandardNear(Standard::Duration(Standard::ProfileFor(1101)),.21f) &&
    StandardNear(Standard::Duration(Standard::ProfileFor(1103)),.27f) &&
    Standard::ImpactAt(Standard::ProfileFor(3102),1,1).frame==4 &&
    StandardNear(Standard::Duration(Standard::ProfileFor(3102)),.32f) &&
    !Standard::ImpactAt(Standard::ProfileFor(3102),1.32f,1).visible,
    "Quick, break and elemental mid frames begin at impact, finish independently and cannot damage during flight");
static_assert(!Standard::FlightPhaseAt(.29f,.3f,.7f).visible &&
    Standard::FlightPhaseAt(.3f,.3f,.7f).visible &&
    StandardNear(Standard::FlightPhaseAt(.5f,.3f,.7f).progress,.5f) &&
    !Standard::FlightPhaseAt(.7f-.0000005f,.3f,.7f).visible &&
    Standard::FlightPhaseAt(.7f,.3f,.79f).visible &&
    !Standard::FlightPhaseAt(.79f,.3f,.79f).visible &&
    StandardNear(Standard::StraightFlight({true,0,1},0,0,100,60).x,100) &&
    StandardNear(Standard::StraightFlight({true,0,1},0,0,100,60).y,60),
    "Fan outer-cell arrows survive central impact and all tips terminate at the exact captured body arrival");
constexpr bool FanRingCoverage()
{
    int atCenter=0,atOuter=0,total=0;
    BattleSkillArea::ForEachCell(BattleSkillArea::Kind::Splash1,{1,4},{4,4},3,20,20,[&](BattleSkillArea::Cell cell)
    {
        ++total;const float arrival=BattleSkillArea::SplashArrival(.3f,.4f,BattleSkillArea::Distance({cell.x,cell.y},{4,4}));
        if(Standard::FlightPhaseAt(.7f,.3f,arrival).visible)++atCenter;
        if(Standard::FlightPhaseAt(.79f,.3f,arrival).visible)++atOuter;
    });
    return total==5 && atCenter==4 && atOuter==0;
}
static_assert(FanRingCoverage(),"All five real fan cells, including empty ones, use the unchanged center/ring schedule");
void CompileStandardPaths(HDC dc,HDC scratch,HBITMAP bitmap)
{
    Standard::Action action;action.profile=Standard::ProfileFor(2302);action.cells={{4,4,.7f},{4,5,.79f}};
    action.release=.3f;action.arrival=.7f;action.flight=.4f;action.elapsed=.72f;
    StandardSkillRenderer::Options options;options.bitmap=bitmap;options.projectile=bitmap;options.contact=bitmap;
    StandardSkillRenderer::Draw(dc,scratch,action,options);
    options.front=true;StandardSkillRenderer::Draw(dc,scratch,action,options);
    action.profile=Standard::ProfileFor(3301);action.elapsed=.5f;
    StandardSkillRenderer::Draw(dc,scratch,action,options);
    options.bitmap=nullptr;action.profile=Standard::ProfileFor(1101);
    StandardSkillRenderer::Draw(dc,scratch,action,options);
}
