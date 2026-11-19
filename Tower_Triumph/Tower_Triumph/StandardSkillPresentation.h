#pragma once
#include "BattleSkillArea.h"
#include <cmath>
#include <cstdint>
#include <string>

// The remaining thirty skills keep gameplay timing/geometry and select an
// explicit visual motion. Signature skills intentionally do not resolve here.
namespace StandardSkillPresentation
{
    using Element=BattleActionTiming::ElementVisual;
    enum class Class { None, Knight, Archer, Mage };
    enum class Tier { Basic, Middle };
    enum class Motion { None, Slash, Break, Smash, Arrow, BurstArrow, PiercingArrow, FanVolley, RadiantArrow,
        Bolt, VineWhip, GrowingRay, FlameBurst, ForwardWave, ThornField, Smite, ShadowSpike };
    struct Profile { int skillID=0;Class owner=Class::None;Tier tier=Tier::Basic;Motion motion=Motion::None;Element element=Element::Neutral; };
    constexpr Profile ProfileFor(int id)
    {
        switch(id)
        {
        case 1101:return {id,Class::Knight,Tier::Basic,Motion::Slash,Element::Fire};
        case 1103:return {id,Class::Knight,Tier::Middle,Motion::Break,Element::Fire};
        case 1201:return {id,Class::Knight,Tier::Basic,Motion::Slash,Element::Water};
        case 1203:return {id,Class::Knight,Tier::Middle,Motion::Break,Element::Water};
        case 1301:return {id,Class::Knight,Tier::Basic,Motion::Slash,Element::Grass};
        case 1303:return {id,Class::Knight,Tier::Middle,Motion::Smash,Element::Grass};
        case 1401:return {id,Class::Knight,Tier::Basic,Motion::Slash,Element::Light};
        case 1402:return {id,Class::Knight,Tier::Middle,Motion::Break,Element::Light};
        case 1501:return {id,Class::Knight,Tier::Basic,Motion::Slash,Element::Dark};
        case 1502:return {id,Class::Knight,Tier::Middle,Motion::Break,Element::Dark};
        case 2101:return {id,Class::Archer,Tier::Basic,Motion::Arrow,Element::Fire};
        case 2102:return {id,Class::Archer,Tier::Middle,Motion::BurstArrow,Element::Fire};
        case 2201:return {id,Class::Archer,Tier::Basic,Motion::Arrow,Element::Water};
        case 2202:return {id,Class::Archer,Tier::Middle,Motion::PiercingArrow,Element::Water};
        case 2301:return {id,Class::Archer,Tier::Basic,Motion::Arrow,Element::Grass};
        case 2302:return {id,Class::Archer,Tier::Middle,Motion::FanVolley,Element::Grass};
        case 2401:return {id,Class::Archer,Tier::Basic,Motion::Arrow,Element::Light};
        case 2402:return {id,Class::Archer,Tier::Middle,Motion::RadiantArrow,Element::Light};
        case 2501:return {id,Class::Archer,Tier::Basic,Motion::Arrow,Element::Dark};
        case 2502:return {id,Class::Archer,Tier::Middle,Motion::PiercingArrow,Element::Dark};
        case 3101:return {id,Class::Mage,Tier::Basic,Motion::Bolt,Element::Fire};
        case 3102:return {id,Class::Mage,Tier::Middle,Motion::FlameBurst,Element::Fire};
        case 3201:return {id,Class::Mage,Tier::Basic,Motion::Bolt,Element::Water};
        case 3202:return {id,Class::Mage,Tier::Middle,Motion::ForwardWave,Element::Water};
        case 3301:return {id,Class::Mage,Tier::Basic,Motion::VineWhip,Element::Grass};
        case 3302:return {id,Class::Mage,Tier::Middle,Motion::ThornField,Element::Grass};
        case 3401:return {id,Class::Mage,Tier::Basic,Motion::GrowingRay,Element::Light};
        case 3402:return {id,Class::Mage,Tier::Middle,Motion::Smite,Element::Light};
        case 3501:return {id,Class::Mage,Tier::Basic,Motion::Bolt,Element::Dark};
        case 3502:return {id,Class::Mage,Tier::Middle,Motion::ShadowSpike,Element::Dark};
        default:return {};
        }
    }
    constexpr bool Valid(Profile p){return p.owner!=Class::None;}
    constexpr bool Middle(Profile p){return p.tier==Tier::Middle;}
    constexpr bool Knight(Profile p){return p.owner==Class::Knight;}
    constexpr bool Fan(Profile p){return p.motion==Motion::FanVolley;}
    constexpr bool MirrorImpact(Profile p,float facing)
    {return facing<0 && (Knight(p) || (p.element==Element::Water &&
        (p.motion==Motion::PiercingArrow || p.motion==Motion::ForwardWave)));}
    constexpr bool IsBitmap(int id){return (id<=-500 && id>=-504)||(id<=-510 && id>=-514);}
    constexpr bool IsKnightBitmap(int id){return id<=-500 && id>=-504;}
    constexpr int BitmapID(Profile p)
    {
        const int element=BattleActionTiming::ElementIndex(p.element);
        return element<0?0:Knight(p)?-500-element:Middle(p)?-510-element:0;
    }
    inline std::wstring Resource(int id)
    {
        if(!IsBitmap(id))return L"";
        const int index=(IsKnightBitmap(id)?-500:-510)-id;
        return std::wstring(IsKnightBitmap(id)?L"Resource\\Skill_Warrior_Standard_":L"Resource\\Skill_Element_Mid_")+
            BattleActionTiming::ElementSuffix(static_cast<Element>(index+1))+L".bmp";
    }
    struct Anchors { float x,y,split; };
    constexpr Anchors ImpactAnchor(Profile p)
    {return Knight(p)?Anchors{64,64,128}:Anchors{64,104,96};}
    constexpr Anchors AuraAnchor(Element){return {64,64,128};}
    constexpr int Columns=4,Rows=3,CellSize=128;
    constexpr float QuickPoseSeconds=.035f,BreakPoseSeconds=.045f,MidPoseSeconds=.04f;
    constexpr float Duration(Profile p){return Knight(p)?6*(Middle(p)?BreakPoseSeconds:QuickPoseSeconds):Middle(p)?8*MidPoseSeconds:.20f;}
    constexpr int FlightSize(Profile p){return Middle(p)?40:32;}
    constexpr int AuraSize(Profile p){return p.owner==Class::Archer?24:44;}
    constexpr int MainSize(Profile p)
    {
        if(Knight(p))return Middle(p)?108:84;
        if(p.motion==Motion::BurstArrow || p.motion==Motion::FlameBurst)return 128;
        if(p.motion==Motion::PiercingArrow || p.motion==Motion::ForwardWave)return 72;
        return p.motion==Motion::RadiantArrow?96:112;
    }
    constexpr int CellSizeFor(Profile p,bool center,BattleSkillArea::Kind area)
    {return area==BattleSkillArea::Kind::Line?72:center?MainSize(p):64;}
    constexpr float Clamp(float t){return t<0?0:t>1?1:t;}
    struct ImpactSample { bool visible;int frame;float alpha; };
    constexpr ImpactSample ImpactAt(Profile p,float elapsed,float arrival)
    {
        if(!Valid(p) || !BattleSkillArea::IsDue(elapsed,arrival) || elapsed>=arrival+Duration(p))return {};
        const float age=elapsed>arrival?elapsed-arrival:0;
        const float step=Knight(p)?(Middle(p)?BreakPoseSeconds:QuickPoseSeconds):MidPoseSeconds;
        const int max=Knight(p)?6:8;
        const int raw=static_cast<int>((age+BattleSkillArea::EventEpsilon)/step);
        const int offset=Knight(p)?(Middle(p)?6:0):4;
        const float fade=age<Duration(p)*.75f?1:Clamp((Duration(p)-age)/(Duration(p)*.25f));
        return {true,offset+(raw<max?raw:max-1),fade};
    }
    struct Action
    {
        Profile profile;
        BattleSkillArea::Kind area=BattleSkillArea::Kind::Single;
        std::vector<BattleSkillArea::Cell> cells;
        BattleSkillArea::Point center;
        BattleSkillArea::LinePath linePath;
        float elapsed=0,release=.30f,arrival=.30f,flight=0;
        float startX=0,startY=0,targetX=0,targetY=0,facing=1;
        std::uint64_t id=0;
    };
    inline float EndSeconds(const Action& action)
    {return (action.cells.empty()?action.arrival:action.cells.back().arrivalSeconds)+Duration(action.profile);}
    struct FlightSample { bool visible;int frame;float x,y,dx,dy,progress; };
    struct FlightPhase { bool visible;int frame;float progress; };
    constexpr FlightPhase FlightPhaseAt(float elapsed,float release,float arrival)
    {
        if(!BattleSkillArea::IsDue(elapsed,release) || BattleSkillArea::IsDue(elapsed,arrival) || !(arrival>release))return {};
        const float age=elapsed>release?elapsed-release:0;
        return {true,static_cast<int>((age+BattleSkillArea::EventEpsilon)/BattleActionTiming::ProjectileFrameSeconds)%4,
            Clamp(age/(arrival-release))};
    }
    constexpr FlightSample StraightFlight(FlightPhase phase,float sx,float sy,float tx,float ty)
    {return {phase.visible,phase.frame,sx+(tx-sx)*phase.progress,sy+(ty-sy)*phase.progress,tx-sx,ty-sy,phase.progress};}
    inline FlightSample FlightAt(const Action& action,float elapsed,float targetX,float targetY,float arrival,bool line)
    {
        const auto phase=FlightPhaseAt(elapsed,action.release,arrival);
        if(Knight(action.profile) || !phase.visible)return {};
        if(line)
        {
            const auto point=BattleSkillArea::SampleLinePath(action.linePath,phase.progress);
            return {true,phase.frame,point.x,point.y,point.dx,point.dy,phase.progress};
        }
        return StraightFlight(phase,action.startX,action.startY,targetX,targetY);
    }
    struct TrailPoint {float x,y;};
    inline TrailPoint VinePoint(float startX,float startY,float endX,float endY,int index)
    {
        constexpr float curls[9]={0,.5f,1,.5f,0,-.5f,-1,-.5f,0};
        const int i=index<0?0:index>8?8:index;
        const float t=i/8.0f,dx=endX-startX,dy=endY-startY;
        const float length=std::sqrt(dx*dx+dy*dy);
        const float curl=curls[i]*5.0f;
        return {startX+dx*t+(length>0?-dy/length*curl:0),startY+dy*t+(length>0?dx/length*curl:0)};
    }
}
