#include "../Tower_Triumph/Tower_Triumph/UltimateSkillRenderer.h"
#include "../Tower_Triumph/Tower_Triumph/CharacterMotion.h"
namespace Present = SkillPresentation;
constexpr bool NearUltimate(float a,float b){return a>b-.00001f && a<b+.00001f;}
static_assert(Present::Resolve(3103,true)==Present::Kind::Meteor && Present::Resolve(3403,true)==Present::Kind::Sanctuary &&
    Present::Resolve(3103,false)==Present::Kind::Normal && Present::Resolve(3403,false)==Present::Kind::Normal &&
    Present::Requested(3102)==Present::Kind::Normal && Present::Requested(3502)==Present::Kind::Normal,
    "The existing Mage profiles and their missing-art fallback must remain intact");
static_assert(NearUltimate(.3f+Present::DeliverySeconds(Present::Kind::Meteor),1.02f) &&
    NearUltimate(.3f+Present::DeliverySeconds(Present::Kind::Sanctuary),1.10f) &&
    .3f+Present::MeteorFallSeconds>CharacterMotion::ActionTiming::SkillSeconds,
    "Sky delivery extends beyond actor recovery and commits at the dedicated impact boundary");
static_assert(Present::At(Present::Kind::Meteor,0).frame==0 &&
    Present::At(Present::Kind::Meteor,0).coreX==-192 && Present::At(Present::Kind::Meteor,0).coreY==-224 &&
    Present::At(Present::Kind::Meteor,.72f).frame==4 &&
    Present::At(Present::Kind::Meteor,.72f-.0000005f).frame==4 &&
    Present::At(Present::Kind::Sanctuary,.8f).frame==4 &&
    Present::At(Present::Kind::Sanctuary,Present::Lifetime(Present::Kind::Sanctuary)).frame==-1,
    "Main art impact and damage use the same epsilon; preparation is distinct and terminal art disappears");
constexpr bool UltimateSchedule(Present::Kind kind,float frameSeconds)
{
    BattleActionTiming::ActionTimeline timeline{{0,.3f,false},Present::DeliverySeconds(kind),true,false,false};
    BattleSkillArea::Cell cells[13]{};int count=0,next=0,launches=0,impacts=0;
    BattleSkillArea::ForEachCell(BattleSkillArea::Kind::Splash2,{1,4},{5,4},4,20,20,
        [&](BattleSkillArea::Cell cell){cell.arrivalSeconds=BattleSkillArea::SplashArrival(.3f,Present::DeliverySeconds(kind),
            BattleSkillArea::Distance({cell.x,cell.y},{5,4}));cells[count++]=cell;});
    float elapsed=0;
    for(int frame=0;frame<300 && elapsed<2.0f;++frame)
    {
        float remaining=frameSeconds;int iterations=0;
        while(remaining>0)
        {
            if(++iterations>32)return false;
            const auto step=BattleSkillArea::AdvanceTimeline(timeline,elapsed,remaining,next<count?cells[next].arrivalSeconds:-1);
            if(step.event==BattleActionTiming::ActionEvent::Launch)++launches;
            if(step.event==BattleActionTiming::ActionEvent::Impact)++impacts;
            while(next<count && BattleSkillArea::IsDue(elapsed,cells[next].arrivalSeconds))
            {
                if(!timeline.launched || !timeline.impactCommitted || !NearUltimate(elapsed,cells[next].arrivalSeconds) ||
                    Present::At(kind,elapsed-.3f).frame<4)return false;
                ++next;
            }
            remaining=step.afterEvent;
            if(step.beforeEvent==0 && step.event==BattleActionTiming::ActionEvent::None)return false;
        }
    }
    return count==13 && next==13 && launches==1 && impacts==1;
}
static_assert(UltimateSchedule(Present::Kind::Meteor,1.0f/30) && UltimateSchedule(Present::Kind::Sanctuary,1.0f/60) &&
    UltimateSchedule(Present::Kind::Meteor,1.0f/120) && UltimateSchedule(Present::Kind::Sanctuary,2.0f),
    "Large dt and normal frame rates must deliver launch, unique impact and all 13 ring cells exactly once");
static_assert(Present::MeteorBitmapID!=Present::SanctuaryBitmapID &&
    !BattleActionTiming::IsContactEffect(Present::MeteorBitmapID) &&
    !BattleActionTiming::IsProjectileEffect(Present::SanctuaryBitmapID) &&
    Present::Anchors(Present::Kind::Meteor).groundStartY<Present::Anchors(Present::Kind::Meteor).groundY &&
    Present::Anchors(Present::Kind::Meteor).groundY<Present::CellSize,
    "Ultimate resources and the air/ground split stay separate from contact and projectile atlases");
static_assert(Present::Anchors(Present::Kind::Meteor).coreX==86 && Present::Anchors(Present::Kind::Meteor).coreY==76 &&
    Present::Anchors(Present::Kind::Meteor).groundY==114 && Present::Anchors(Present::Kind::Meteor).groundStartY==108 &&
    Present::Anchors(Present::Kind::Sanctuary).groundY==104 && Present::Anchors(Present::Kind::Sanctuary).groundStartY==96,
    "Each authored sheet must retain its independently measured core and ground anchor");
static_assert(NearUltimate(Present::At(Present::Kind::Meteor,.36f).coreX,-144.0f) &&
    NearUltimate(Present::At(Present::Kind::Meteor,.36f).coreY,-168.0f),
    "Meteor acceleration must keep both axes on the same diagonal as the authored tail");
void CompileUltimateRenderer(HDC destination,HDC scratch,HBITMAP bitmap)
{
    auto cells=BattleSkillArea::BuildCells(BattleSkillArea::Kind::Splash2,{1,4},{5,4},4,20,20);
    UltimateSkillRenderer::Options options;
    options.bitmap=bitmap;options.kind=Present::Kind::Meteor;
    UltimateSkillRenderer::Draw(destination,scratch,cells,1.02f,.3f,options);
    options.kind=Present::Kind::Sanctuary;
    UltimateSkillRenderer::Draw(destination,scratch,cells,.6f,.3f,options);
    options.kind=Present::Kind::Meteor;options.front=true;
    UltimateSkillRenderer::Draw(destination,scratch,cells,.6f,.3f,options);
}

constexpr bool AllProfiles()
{
    constexpr int ids[]={3103,3403,1102,1202,1302,1403,1503,2103,2203,2303,2503,2403,3203,3303,3503};
    int resources[19]{};int count=0;
    for(int id:ids)
    {
        const auto kind=Present::Requested(id);
        if(kind==Present::Kind::Normal || Present::Resolve(id,false,true)!=Present::Kind::Normal ||
            Present::Resolve(id,true,true)!=kind || Present::Resource(kind)[0]==L'\0')return false;
        const int bitmap=Present::BitmapID(kind);
        if(!Present::IsUltimateBitmap(bitmap) || Present::KindForBitmap(bitmap)!=kind ||
            BattleActionTiming::IsContactEffect(bitmap) || BattleActionTiming::IsProjectileEffect(bitmap))return false;
        for(int i=0;i<count;++i)if(resources[i]==bitmap)return false;
        resources[count++]=bitmap;
        if(Present::IsSnipe(kind))
        {
            if(Present::Resolve(id,true,false)!=Present::Kind::Normal)return false;
            const int flight=Present::FlightBitmapID(kind);
            if(!Present::IsFlightBitmap(flight)||Present::KindForBitmap(flight)!=kind)return false;
            for(int i=0;i<count;++i)if(resources[i]==flight)return false;
            resources[count++]=flight;
        }
    }
    return count==19 && Present::Requested(1103)==Present::Kind::Normal &&
        Present::Requested(1203)==Present::Kind::Normal && Present::Requested(1303)==Present::Kind::Normal &&
        Present::Requested(1401)==Present::Kind::Normal && Present::Requested(2101)==Present::Kind::Normal;
}
static_assert(AllProfiles(), "All fifteen profiles use explicit skill IDs and unique resource IDs; snipes require both atlases");
static_assert(static_cast<int>(Present::Kind::LightRain)==12 &&
    Present::BitmapID(Present::Kind::TidalBlast)==-402 && Present::BitmapID(Present::Kind::RootStorm)==-403 &&
    Present::BitmapID(Present::Kind::AbyssBlast)==-404 && !Present::IsUltimateBitmap(-405) &&
    Present::Requested(3201)==Present::Kind::Normal && Present::Requested(3301)==Present::Kind::Normal &&
    Present::Requested(3501)==Present::Kind::Normal,
    "Appending three Mage profiles must retain every old enum/resource ID and ordinary comparison skill");
static_assert(Present::At(Present::Kind::KnightLight,-.3f).phase==Present::Phase::Charge &&
    Present::At(Present::Kind::KnightLight,-.01f).frame==3 &&
    Present::At(Present::Kind::KnightLight,0).frame==4 &&
    Present::At(Present::Kind::SnipeFire,-.1f,.2f).phase==Present::Phase::Charge &&
    Present::At(Present::Kind::SnipeFire,.1f,.2f).phase==Present::Phase::Flight &&
    Present::At(Present::Kind::SnipeFire,.2f,.2f).frame==4,
    "Knight contact starts the slash immediately; sniper windup, flight and impact remain distinct");
static_assert(Present::KnightChargeOffset(0).x==-16 && Present::KnightChargeOffset(.1f).y==-33 &&
    Present::KnightChargeOffset(.2f).y==-54 && Present::KnightChargeOffset(.25f).x==9 &&
    Present::KnightChargeSize==80 && Present::At(Present::Kind::KnightLight,-.00001f).opacity<.66f,
    "Knight charge follows the six measured blade poses and remains translucent enough to show the actual sword");
static_assert(Present::DeliverySeconds(Present::Kind::KnightWater)==0 &&
    Present::LineWaveProgress(0,3)==0 && NearUltimate(Present::LineWaveProgress(.09f,3),.5f) &&
    Present::LineWaveProgress(.18f,3)==1 && Present::LineWaveProgress(0,1)==1 &&
    NearUltimate(Present::SnipeFlightSeconds(192),.32f) && NearUltimate(Present::SnipeFlightSeconds(1),.14f),
    "Water's wave preserves .09s per cell and close/far snipes use their own distance-based speed");
static_assert(NearUltimate(Present::DeliverySeconds(Present::Kind::LightRain),.70f) &&
    Present::At(Present::Kind::LightRain,.18f).frame==0 &&
    Present::At(Present::Kind::LightRain,.70f).frame==4 &&
    Present::Front(Present::Kind::LightRain,Present::Phase::Flight) &&
    !Present::Front(Present::Kind::LightRain,Present::Phase::Impact),
    "Light rain launches upward then falls before any impact, with airborne arrows above units");
static_assert(!Present::RainArrowAt(.779f,1).visible &&
    Present::RainArrowAt(.78f,1).visible && NearUltimate(Present::RainArrowAt(.78f,1).offsetX,-36) &&
    NearUltimate(Present::RainArrowAt(.78f,1).offsetY,-87) &&
    Present::RainArrowAt(.89f,1).frame==1 && NearUltimate(Present::RainArrowAt(.89f,1).offsetY,-43.5f) &&
    Present::RainArrowAt(.99999f,1).visible && Present::RainArrowAt(.99999f,1).offsetY>-.005f &&
    !Present::RainArrowAt(1-.0000005f,1).visible && !Present::RainArrowAt(1,1).visible &&
    !Present::RainArrowAt(2,1).visible,
    "A cell arrow starts .22s before its arrival, reaches the body tip and disappears on the exact damage epsilon");
constexpr bool RainCellArrows()
{
    int total=0,atFirst=0,atCenter=0,atSecond=0,atLast=0;
    BattleSkillArea::ForEachCell(BattleSkillArea::Kind::Splash2,{11,6},{15,6},4,25,20,
        [&](BattleSkillArea::Cell cell)
        {
            const float arrival=BattleSkillArea::SplashArrival(.3f,.7f,BattleSkillArea::Distance({cell.x,cell.y},{15,6}));
            if(!Present::RainArrowAt(arrival-.11f,arrival).visible)return;
            ++total;
            if(Present::RainArrowAt(.78f,arrival).visible)++atFirst;
            if(Present::RainArrowAt(1,arrival).visible)++atCenter;
            if(Present::RainArrowAt(1.09f,arrival).visible)++atSecond;
            if(Present::RainArrowAt(1.18f,arrival).visible)++atLast;
        });
    return total==13 && atFirst==1 && atCenter==12 && atSecond==8 && atLast==0;
}
static_assert(RainCellArrows(),
    "Every actual footprint cell, including empty and outer-ring cells, has one arrow with independent same-ring arrival");
constexpr bool FamilyTiming(Present::Kind kind,bool line,float frameSeconds)
{
    const bool knight=Present::IsKnight(kind),single=Present::IsSnipe(kind);
    const float flight=single?Present::SnipeFlightSeconds(192):Present::DeliverySeconds(kind);
    BattleActionTiming::ActionTimeline timeline{{0,.3f,false},flight,!knight,false,false};
    float arrivals[13]{};const int count=single?1:line?3:(knight||kind==Present::Kind::TidalBlast)?5:13;
    for(int i=0;i<count;++i)
    {
        const int ring=single?0:line?i:i==0?0:i<5?1:2;
        arrivals[i]=.3f+flight+ring*BattleSkillArea::SpreadTileSeconds;
    }
    float elapsed=0;int next=0,impacts=0,launches=0;
    for(int frame=0;frame<300 && elapsed<2;++frame)
    {
        float remaining=frameSeconds;int iterations=0;
        while(remaining>0)
        {
            if(++iterations>32)return false;
            const auto step=BattleSkillArea::AdvanceTimeline(timeline,elapsed,remaining,next<count?arrivals[next]:-1);
            if(step.event==BattleActionTiming::ActionEvent::Launch)++launches;
            if(step.event==BattleActionTiming::ActionEvent::Impact)++impacts;
            while(next<count && BattleSkillArea::IsDue(elapsed,arrivals[next]))
            {
                if(!timeline.impactCommitted || !NearUltimate(elapsed,arrivals[next]) ||
                    Present::At(kind,elapsed-.3f,flight).phase!=Present::Phase::Impact)return false;
                ++next;
            }
            remaining=step.afterEvent;
            if(step.beforeEvent==0 && step.event==BattleActionTiming::ActionEvent::None)return false;
        }
    }
    return next==count && impacts==1 && launches==(knight?0:1);
}
static_assert(FamilyTiming(Present::Kind::KnightLight,false,1.0f/60) &&
    FamilyTiming(Present::Kind::KnightWater,true,2) && FamilyTiming(Present::Kind::SnipeFire,false,1.0f/30) &&
    FamilyTiming(Present::Kind::SnipeDark,false,2) && FamilyTiming(Present::Kind::LightRain,false,1.0f/120) &&
    FamilyTiming(Present::Kind::LightRain,false,2),
    "Knight/ranged/Rain transitions and target counts survive fine and multi-boundary updates without early or duplicate hits");
static_assert(FamilyTiming(Present::Kind::TidalBlast,false,1.0f/30) && FamilyTiming(Present::Kind::TidalBlast,false,2) &&
    FamilyTiming(Present::Kind::RootStorm,false,1.0f/60) && FamilyTiming(Present::Kind::RootStorm,false,2) &&
    FamilyTiming(Present::Kind::AbyssBlast,false,1.0f/120) && FamilyTiming(Present::Kind::AbyssBlast,false,2),
    "New Mage profiles must not hit during preparation and deliver all actual five/thirteen cells once across large dt");
static_assert(NearUltimate(.3f+Present::DeliverySeconds(Present::Kind::TidalBlast),.95f) &&
    NearUltimate(.3f+Present::DeliverySeconds(Present::Kind::RootStorm),1.0f) &&
    NearUltimate(.3f+Present::DeliverySeconds(Present::Kind::AbyssBlast),1.1f) &&
    Present::At(Present::Kind::TidalBlast,.649f).frame==3 && Present::At(Present::Kind::TidalBlast,.65f).frame==4 &&
    Present::At(Present::Kind::RootStorm,.70f).frame==4 && Present::At(Present::Kind::AbyssBlast,.80f).frame==4 &&
    Present::At(Present::Kind::RootStorm,.35f).coreY==0 &&
    Present::MainDrawSizeFor(Present::Kind::TidalBlast)==224 && Present::MainDrawSizeFor(Present::Kind::RootStorm)==240 &&
    !Present::Front(Present::Kind::AbyssBlast,Present::Phase::Flight) &&
    !Present::Front(Present::Kind::TidalBlast,Present::Phase::Impact),
    "Ground preparation remains fixed; each class effect uses its authored size and exact center-impact timing");
constexpr bool SummonCells(Present::Kind kind,bool edge)
{
    const auto area=kind==Present::Kind::TidalBlast?BattleSkillArea::Kind::Splash1:BattleSkillArea::Kind::Splash2;
    const BattleSkillArea::Point center=edge?BattleSkillArea::Point{0,0}:BattleSkillArea::Point{5,5};
    const float impact=.3f+Present::DeliverySeconds(kind);int count=0;bool valid=true;
    BattleSkillArea::ForEachCell(area,{1,1},center,4,20,20,[&](BattleSkillArea::Cell cell)
    {
        ++count;const float arrival=impact+BattleSkillArea::Distance({cell.x,cell.y},center)*BattleSkillArea::SpreadTileSeconds;
        const auto before=Present::CellSummonAt(kind,cell.x,cell.y,arrival-.0001f,arrival);
        const auto at=Present::CellSummonAt(kind,cell.x,cell.y,arrival-.0000005f,arrival);
        const auto tail=Present::CellSummonAt(kind,cell.x,cell.y,arrival+.38f,arrival);
        valid=valid && !before.visible && at.visible && at.frame==0 && at.alpha>0 && !tail.visible &&
            arrival+Present::CellSummonSeconds<=.3f+Present::Lifetime(kind);
    });
    return valid && count==(edge?(area==BattleSkillArea::Kind::Splash1?3:6):(area==BattleSkillArea::Kind::Splash1?5:13));
}
static_assert(SummonCells(Present::Kind::TidalBlast,false) && SummonCells(Present::Kind::TidalBlast,true) &&
    SummonCells(Present::Kind::RootStorm,false) && SummonCells(Present::Kind::RootStorm,true) &&
    SummonCells(Present::Kind::AbyssBlast,false) && SummonCells(Present::Kind::AbyssBlast,true) &&
    !Present::CellSummonAt(Present::Kind::Meteor,5,5,1,1).visible &&
    !Present::CellSummonAt(Present::Kind::LightRain,5,5,1,1).visible,
    "Cell eruptions begin on each damage epsilon, include empty/clipped-edge cells and finish within the main lifetime without altering old profiles");
constexpr Present::CellSummonSample RootShapeFixture{true,0,40,100,0,1,1};
static_assert(Present::RootStemSegments==5 && Present::RootStemWidth(0)==3 &&
    Present::RootStemWidth(1)==2 && Present::RootStemWidth(2)==2 && Present::RootStemWidth(4)==1 &&
    Present::RootStemPoint(RootShapeFixture,1,0).y==0 &&
    NearUltimate(Present::RootStemPoint(RootShapeFixture,1,5).y,-70) &&
    NearUltimate(Present::RootStemPoint(RootShapeFixture,-1,5).y,-65.8f) &&
    Present::RootBranchPoint(RootShapeFixture,1,0).x==Present::RootStemPoint(RootShapeFixture,1,2).x &&
    Present::RootBranchPoint(RootShapeFixture,1,2).x==12 &&
    NearUltimate(Present::CellSummonAt(Present::Kind::RootStorm,5,5,1,1).shapeAlpha,.42f) &&
    NearUltimate(Present::CellSummonAt(Present::Kind::TidalBlast,5,5,1,1).shapeAlpha,.70f) &&
    NearUltimate(Present::CellSummonAt(Present::Kind::AbyssBlast,5,5,1,1).shapeAlpha,.70f) &&
    NearUltimate(Present::CellSummonAt(Present::Kind::RootStorm,5,5,1,1).alpha,.72f),
    "Thin branching roots taper and use reduced height/opacity without changing the burst sprite, Water/Dark or arrival timing");
