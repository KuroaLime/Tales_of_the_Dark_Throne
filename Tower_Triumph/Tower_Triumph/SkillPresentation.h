#pragma once
#include "BattleSkillArea.h"

namespace SkillPresentation
{
    enum class Kind
    {
        Normal, Meteor, Sanctuary,
        KnightFire, KnightWater, KnightGrass, KnightLight, KnightDark,
        SnipeFire, SnipeWater, SnipeGrass, SnipeDark, LightRain,
        TidalBlast, RootStorm, AbyssBlast
    };
    enum class Phase { Hidden, Charge, Flight, Impact };
    constexpr int MeteorSkillID=3103,SanctuarySkillID=3403;
    constexpr int MeteorBitmapID=-400,SanctuaryBitmapID=-401;
    constexpr int Columns=4,Rows=3,CellSize=128,FlightDirections=16,FlightFrames=4;
    constexpr bool IsKnight(Kind kind){return kind>=Kind::KnightFire && kind<=Kind::KnightDark;}
    constexpr bool IsSnipe(Kind kind){return kind>=Kind::SnipeFire && kind<=Kind::SnipeDark;}
    constexpr bool IsRain(Kind kind){return kind==Kind::LightRain;}
    constexpr bool IsCellSummon(Kind kind){return kind>=Kind::TidalBlast && kind<=Kind::AbyssBlast;}
    constexpr bool StartsOnCommand(Kind kind){return IsKnight(kind)||IsSnipe(kind)||IsRain(kind);}
    struct SpriteAnchors { int groundX,groundY,groundStartY; float coreX,coreY; int chargeX=64,chargeY=104; };
    constexpr SpriteAnchors Anchors(Kind kind)
    {
        if(kind==Kind::Meteor)return {64,114,108,86,76};
        if(kind==Kind::Sanctuary)return {64,104,96,64,48};
        if(kind==Kind::KnightFire)return {64,111,104,64,111,64,64};
        if(kind==Kind::KnightWater)return {64,114,108,64,114,64,64};
        if(kind==Kind::KnightGrass)return {76,112,104,76,112,64,70};
        if(kind==Kind::KnightLight)return {64,111,104,64,111,64,64};
        if(kind==Kind::KnightDark)return {73,108,100,73,108,67,72};
        if(kind==Kind::SnipeFire)return {64,64,128,114.5f,65.5f,64,64};
        if(kind==Kind::SnipeWater)return {64,62,128,117.5f,63.5f,64,64};
        if(kind==Kind::SnipeGrass)return {64,61,128,116.5f,64.5f,64,64};
        if(kind==Kind::SnipeDark)return {64,64,128,115.5f,67.5f,64,64};
        if(kind==Kind::LightRain)return {64,109,102,64,122,64,122};
        if(kind==Kind::TidalBlast)return {64,114,106,64,114};
        if(kind==Kind::RootStorm)return {64,115,107,64,115};
        if(kind==Kind::AbyssBlast)return {64,114,106,64,114};
        if(IsSnipe(kind))return {64,64,128,110,64,64,64};
        return {64,104,96,64,104,64,104};
    }
    constexpr float MeteorFallSeconds=.72f,SanctuaryChargeSeconds=.80f;
    constexpr float TidalChargeSeconds=.65f,RootChargeSeconds=.70f,AbyssChargeSeconds=.80f;
    constexpr float WindupSeconds=.30f,RainRiseSeconds=.18f,RainFallSeconds=.52f;
    constexpr float SnipeSpeed=600.0f,SnipeMinimumSeconds=.14f,SnipePoseSeconds=.05f;
    constexpr float ImpactFrameSeconds=.08f,ImpactSeconds=8*ImpactFrameSeconds;
    constexpr int MainDrawSize=240,MeteorDrawSize=192,KnightChargeSize=80,KnightImpactSize=180;
    constexpr int MainDrawSizeFor(Kind kind){return kind==Kind::TidalBlast?224:MainDrawSize;}
    constexpr int SnipeDrawSize=80,SnipeImpactSize=112,RainLaunchSize=48;
    constexpr float RainArrowSeconds=.22f,RainArrowStartX=-36.0f,RainArrowStartY=-87.0f;
    constexpr int RainArrowSize=36;
    constexpr float MeteorStartX=-192.0f,MeteorStartY=-224.0f,RainStartX=-24.0f,RainStartY=-180.0f;
    constexpr Kind Requested(int skillID)
    {
        switch(skillID)
        {
        case 3103:return Kind::Meteor;case 3403:return Kind::Sanctuary;
        case 3203:return Kind::TidalBlast;case 3303:return Kind::RootStorm;case 3503:return Kind::AbyssBlast;
        case 1102:return Kind::KnightFire;case 1202:return Kind::KnightWater;case 1302:return Kind::KnightGrass;
        case 1403:return Kind::KnightLight;case 1503:return Kind::KnightDark;
        case 2103:return Kind::SnipeFire;case 2203:return Kind::SnipeWater;case 2303:return Kind::SnipeGrass;
        case 2503:return Kind::SnipeDark;case 2403:return Kind::LightRain;
        default:return Kind::Normal;
        }
    }
    constexpr Kind Resolve(int skillID,bool bitmapAvailable,bool flightAvailable=true)
    {const auto kind=Requested(skillID);return bitmapAvailable && (!IsSnipe(kind)||flightAvailable)?kind:Kind::Normal;}
    constexpr int BitmapID(Kind kind)
    {
        if(kind==Kind::Meteor)return MeteorBitmapID;if(kind==Kind::Sanctuary)return SanctuaryBitmapID;
        if(IsCellSummon(kind))return -402-(static_cast<int>(kind)-static_cast<int>(Kind::TidalBlast));
        if(IsKnight(kind))return -410-(static_cast<int>(kind)-static_cast<int>(Kind::KnightFire));
        if(IsSnipe(kind))return -420-(static_cast<int>(kind)-static_cast<int>(Kind::SnipeFire));
        return kind==Kind::LightRain?-424:0;
    }
    constexpr int FlightBitmapID(Kind kind)
    {return IsSnipe(kind)?-430-(static_cast<int>(kind)-static_cast<int>(Kind::SnipeFire)):0;}
    constexpr bool IsUltimateBitmap(int id)
    {return (id<=-400 && id>=-404) || (id<=-410 && id>=-414) || (id<=-420 && id>=-424);}
    constexpr bool IsFlightBitmap(int id){return id<=-430 && id>=-433;}
    constexpr Kind KindForBitmap(int id)
    {
        for(int value=1;value<=static_cast<int>(Kind::AbyssBlast);++value)
        {const auto kind=static_cast<Kind>(value);if(BitmapID(kind)==id || (IsSnipe(kind)&&FlightBitmapID(kind)==id))return kind;}
        return Kind::Normal;
    }
    constexpr const wchar_t* Resource(Kind kind,bool flight=false)
    {
        switch(kind)
        {
        case Kind::Meteor:return L"Resource\\Skill_Mage_Meteor_Ultimate.bmp";
        case Kind::Sanctuary:return L"Resource\\Skill_Mage_Sanctuary_Ultimate.bmp";
        case Kind::TidalBlast:return L"Resource\\Skill_Mage_Water_Ultimate.bmp";
        case Kind::RootStorm:return L"Resource\\Skill_Mage_Grass_Ultimate.bmp";
        case Kind::AbyssBlast:return L"Resource\\Skill_Mage_Dark_Ultimate.bmp";
        case Kind::KnightFire:return L"Resource\\Skill_Warrior_Fire_Ultimate.bmp";
        case Kind::KnightWater:return L"Resource\\Skill_Warrior_Water_Ultimate.bmp";
        case Kind::KnightGrass:return L"Resource\\Skill_Warrior_Grass_Ultimate.bmp";
        case Kind::KnightLight:return L"Resource\\Skill_Warrior_Light_Ultimate.bmp";
        case Kind::KnightDark:return L"Resource\\Skill_Warrior_Dark_Ultimate.bmp";
        case Kind::SnipeFire:return flight?L"Resource\\Skill_Archer_Fire_Snipe_Ultimate_Flight.bmp":L"Resource\\Skill_Archer_Fire_Snipe_Ultimate.bmp";
        case Kind::SnipeWater:return flight?L"Resource\\Skill_Archer_Water_Snipe_Ultimate_Flight.bmp":L"Resource\\Skill_Archer_Water_Snipe_Ultimate.bmp";
        case Kind::SnipeGrass:return flight?L"Resource\\Skill_Archer_Grass_Snipe_Ultimate_Flight.bmp":L"Resource\\Skill_Archer_Grass_Snipe_Ultimate.bmp";
        case Kind::SnipeDark:return flight?L"Resource\\Skill_Archer_Dark_Snipe_Ultimate_Flight.bmp":L"Resource\\Skill_Archer_Dark_Snipe_Ultimate.bmp";
        case Kind::LightRain:return L"Resource\\Skill_Archer_Light_Rain_Ultimate.bmp";
        default:return L"";
        }
    }
    constexpr float DeliverySeconds(Kind kind)
    {return kind==Kind::Meteor?MeteorFallSeconds:kind==Kind::Sanctuary?SanctuaryChargeSeconds:
        kind==Kind::TidalBlast?TidalChargeSeconds:kind==Kind::RootStorm?RootChargeSeconds:
        kind==Kind::AbyssBlast?AbyssChargeSeconds:IsRain(kind)?RainRiseSeconds+RainFallSeconds:0;}
    constexpr float SnipeFlightSeconds(float distance){return distance/SnipeSpeed>SnipeMinimumSeconds?distance/SnipeSpeed:SnipeMinimumSeconds;}
    constexpr float ImpactFrameSecondsFor(Kind kind){return IsKnight(kind)?.05f:(IsSnipe(kind)||IsRain(kind))?.06f:ImpactFrameSeconds;}
    constexpr float ImpactSecondsFor(Kind kind){return 8*ImpactFrameSecondsFor(kind);}
    constexpr float Lifetime(Kind kind,float deliveryOverride=-1)
    {return (deliveryOverride>=0?deliveryOverride:DeliverySeconds(kind))+ImpactSecondsFor(kind);}
    constexpr float AreaClamp(float value){return value<0?0:value>1?1:value;}
    constexpr float CellSummonSeconds=.38f,CellSummonFrameSeconds=.04f;
    struct CellSummonSample { bool visible;int frame;float width,height,lean,alpha,shapeAlpha; };
    constexpr CellSummonSample CellSummonAt(Kind kind,int cellX,int cellY,float commandElapsed,float arrivalSeconds)
    {
        if(!IsCellSummon(kind) || !BattleSkillArea::IsDue(commandElapsed,arrivalSeconds) ||
            commandElapsed>=arrivalSeconds+CellSummonSeconds)return {};
        const float age=commandElapsed>arrivalSeconds?commandElapsed-arrivalSeconds:0;
        const float rise=AreaClamp(age/.08f),fade=1-AreaClamp(age/CellSummonSeconds);
        const float shapeFade=1-AreaClamp(age/.28f);
        const float variation=.90f+.20f*((cellX*17+cellY*31)%9)/8.0f;
        const float width=kind==Kind::TidalBlast?36+8*rise:kind==Kind::RootStorm?32+8*rise:46-12*rise;
        const float height=kind==Kind::TidalBlast?48+18*rise:kind==Kind::RootStorm?54+24*rise:54-14*rise;
        const int frame=static_cast<int>((age+BattleSkillArea::EventEpsilon)/CellSummonFrameSeconds);
        return {true,frame<8?frame:7,width*variation,height*variation,
            static_cast<float>((cellX*13+cellY*7)%5-2)*1.5f,.72f*fade*fade,
            .70f*shapeFade*shapeFade*(kind==Kind::RootStorm?.60f:1.0f)};
    }
    struct RootPoint { float x,y; };
    constexpr int RootStemSegments=5;
    constexpr int RootStemWidth(int segment){return segment==0?3:segment<3?2:1;}
    constexpr RootPoint RootStemPoint(const CellSummonSample& state,int side,int index)
    {
        constexpr float spread[6]={6,7,4,6,10,12};
        constexpr float height[6]={0,.17f,.36f,.56f,.78f,1};
        constexpr float lean[6]={0,.2f,.45f,.7f,.9f,1};
        const int point=index<0?0:index>RootStemSegments?RootStemSegments:index;
        const float h=state.height*.70f*(side<0?.94f:1.0f);
        return {side*spread[point]+state.lean*lean[point],-h*height[point]};
    }
    constexpr RootPoint RootBranchPoint(const CellSummonSample& state,int side,int index)
    {
        const auto start=RootStemPoint(state,side,2);
        const float h=state.height*.70f*(side<0?.94f:1.0f);
        return index<=0?start:index==1?RootPoint{start.x+side*5,start.y-h*.10f}:
            RootPoint{start.x+side*8,start.y-h*.16f};
    }
    struct RainArrowSample { bool visible;int frame;float offsetX,offsetY; };
    constexpr RainArrowSample RainArrowAt(float commandElapsed,float arrivalSeconds)
    {
        const float start=arrivalSeconds-RainArrowSeconds;
        if(!BattleSkillArea::IsDue(commandElapsed,start) || BattleSkillArea::IsDue(commandElapsed,arrivalSeconds))
            return {false,0,0,0};
        const float age=commandElapsed>start?commandElapsed-start:0;
        const float remaining=1-AreaClamp(age/RainArrowSeconds);
        const int frame=static_cast<int>((age+BattleSkillArea::EventEpsilon)/BattleActionTiming::ProjectileFrameSeconds)%4;
        return {true,frame,RainArrowStartX*remaining,RainArrowStartY*remaining};
    }
    struct WeaponPoint { float x,y; };
    constexpr WeaponPoint KnightChargeOffset(float commandAge)
    {
        constexpr float points[6][2]={{-16,-6},{-14,-6},{7,-33},{-4,-49},{-9,-54},{9,-37}};
        const int raw=static_cast<int>((commandAge+BattleSkillArea::EventEpsilon)/.05f);
        const int pose=raw<0?0:raw>5?5:raw;
        return {points[pose][0],points[pose][1]};
    }
    constexpr float LineWaveProgress(float releasedAge,int count)
    {const float duration=(count-1)*BattleSkillArea::SpreadTileSeconds;return duration>0?AreaClamp(releasedAge/duration):1.0f;}
    struct Sample { int frame;float coreX,coreY,opacity;bool preparation;Phase phase=Phase::Hidden; };
    constexpr Sample At(Kind kind,float releasedAge,float deliveryOverride=-1)
    {
        if(kind==Kind::Normal || releasedAge>=Lifetime(kind,deliveryOverride))return {-1,0,0,0,false};
        if(releasedAge<0 && !BattleSkillArea::IsDue(releasedAge,0))
        {
            if(!StartsOnCommand(kind)||releasedAge<-WindupSeconds)return {-1,0,0,0,false};
            const float t=AreaClamp((releasedAge+WindupSeconds)/WindupSeconds);
            const auto weapon=IsKnight(kind)?KnightChargeOffset(releasedAge+WindupSeconds):WeaponPoint{0,0};
            return {IsKnight(kind)?static_cast<int>(t*4):0,weapon.x,weapon.y,
                IsKnight(kind)?.20f+.45f*t:.25f+.75f*t,true,Phase::Charge};
        }
        const float delivery=deliveryOverride>=0?deliveryOverride:DeliverySeconds(kind);
        if(!BattleSkillArea::IsDue(releasedAge,delivery))
        {
            if(IsSnipe(kind))return {static_cast<int>(releasedAge/SnipePoseSeconds)%4,0,0,1,true,Phase::Flight};
            if(IsRain(kind))
            {
                if(releasedAge<RainRiseSeconds)return {0,0,0,1,true,Phase::Flight};
                const float t=AreaClamp((releasedAge-RainRiseSeconds)/RainFallSeconds);
                return {static_cast<int>(t*4),RainStartX*(1-t*t),RainStartY*(1-t*t),1,true,Phase::Flight};
            }
            const float t=AreaClamp(releasedAge/delivery);
            if(IsCellSummon(kind))return {static_cast<int>(t*4),0,0,.45f+.55f*t,true,Phase::Flight};
            return {static_cast<int>(t*4),MeteorStartX*(1-t*t),MeteorStartY*(1-t*t),kind==Kind::Sanctuary?.40f+.60f*t:1,true,Phase::Flight};
        }
        const float age=releasedAge>delivery?releasedAge-delivery:0;
        const float duration=ImpactSecondsFor(kind);
        const int frame=static_cast<int>((age+BattleSkillArea::EventEpsilon)/ImpactFrameSecondsFor(kind));
        const float tail=age>duration*.75f?(duration-age)/(duration*.25f):1;
        return {4+(frame<8?frame:7),0,0,tail>0?tail:0,false,Phase::Impact};
    }
    constexpr bool Front(Kind kind,Phase phase)
    {return IsKnight(kind)||IsSnipe(kind)||(IsRain(kind)&&phase!=Phase::Impact)||(kind==Kind::Meteor&&phase==Phase::Flight);}
}
