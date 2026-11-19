"""Extract the real enemy loader and evaluate pair/fallback paths at compile time.

Only bitmap I/O, release, layout caching and frame setup are test doubles.
The decision tree and field assignments are the actual production function.
"""
from pathlib import Path
import argparse

from verify_enemy_animation_integration import ROOT, SOURCE, member


def generate(output: Path) -> None:
    original = (SOURCE / 'Character.cpp').read_text(encoding='utf-8-sig')
    body = member(original, 'Character', 'LoadMonsterSheet', clean=False)
    prefix = r'''
#include <cstddef>
#include <string>
#include "EnemySpriteLayout.h"
#include "BattleActionTiming.h"
struct BITMAP { int bmWidth=0,bmHeight=0; };
using HBITMAP = const BITMAP*;
constexpr int CHARACTER_TRANSPARENT_COLOR=0xff00ff;
struct Character
{
    bool bUseMonsterSheetLayout=false,bUseAuthoredMonsterLayout=false,bSpriteFacesRight=true;
    int MonsterImpactFrame=1,IllustrationTransparentColor=0,CurFrame=7;
    float AnimationElapsedSeconds=.3f;
    bool IsActionEnd=true;
    HBITMAP hSpriteIdle=nullptr,hSpriteWalk=nullptr,hSpriteAttack=nullptr,hSpriteDefend=nullptr,
        hSpriteSkill=nullptr,hSpriteHit=nullptr,hSpriteDead=nullptr,hIllustration=nullptr;
    BITMAP SpriteInfoIdle,SpriteInfoWalk,SpriteInfoAttack,SpriteInfoDefend,SpriteInfoSkill,
        SpriteInfoHit,SpriteInfoDead,SpriteInfoIllustration;
    BITMAP approved{1024,768},portraitImage{128,128},legacy{1536,768};
    std::wstring identity=L"S1_Skeleton";
    bool haveApproved=true,havePortrait=true,haveLegacy=true;
    int approvedLoads=0,portraitLoads=0,legacyLoads=0,approvedDeletes=0,portraitDeletes=0,
        releaseCalls=0,frameSetupCalls=0,layoutCalls=0;
    constexpr void Release()
    {
        ++releaseCalls;bUseAuthoredMonsterLayout=false;bUseMonsterSheetLayout=false;
        hIllustration=nullptr;
    }
    constexpr void DeleteObject(HBITMAP bitmap)
    {
        if(bitmap==&approved)++approvedDeletes;
        if(bitmap==&portraitImage)++portraitDeletes;
    }
    constexpr HBITMAP LoadBitmapFromFile(const std::wstring& path,BITMAP* info)
    {
        HBITMAP result=nullptr;
        if(path==L"Resource\\Monster\\Approved\\"+identity+L"_Animated.bmp")
        {++approvedLoads;if(haveApproved)result=&approved;}
        else if(path==L"Resource\\Monster\\Approved\\"+identity+L"_Portrait.bmp")
        {++portraitLoads;if(havePortrait)result=&portraitImage;}
        else if(path==L"Resource\\Monster\\"+identity+L".bmp")
        {++legacyLoads;if(haveLegacy)result=&legacy;}
        if(result && info)*info=*result;
        return result;
    }
    constexpr void UpdateMaxFrameByState(){++frameSetupCalls;}
    constexpr void PrepareSpriteLayouts(){++layoutCalls;}
    constexpr bool LoadMonsterSheet(const std::wstring&,const std::wstring&,bool);
};
constexpr bool '''
    checks = r'''
constexpr bool SharedAtlas(const Character& c,HBITMAP expected)
{
    return c.hSpriteIdle==expected && c.hSpriteWalk==expected && c.hSpriteAttack==expected &&
        c.hSpriteDefend==expected && c.hSpriteSkill==expected && c.hSpriteHit==expected && c.hSpriteDead==expected;
}
constexpr bool CompletePair(bool originalFacing,bool bowAngel)
{
    Character c;
    if(bowAngel)c.identity=L"S2_BAngel";
    if(!c.LoadMonsterSheet(L"Resource\\Monster",c.identity+L".bmp",originalFacing))return false;
    return c.bUseMonsterSheetLayout && c.bUseAuthoredMonsterLayout && c.bSpriteFacesRight &&
        c.MonsterImpactFrame==(bowAngel?5:4) && c.IllustrationTransparentColor==CHARACTER_TRANSPARENT_COLOR &&
        SharedAtlas(c,&c.approved) && c.hIllustration==&c.portraitImage &&
        c.SpriteInfoIdle.bmWidth==1024 && c.SpriteInfoDead.bmHeight==768 &&
        c.SpriteInfoIllustration.bmWidth==128 && c.SpriteInfoIllustration.bmHeight==128 &&
        c.approvedLoads==1 && c.portraitLoads==1 && c.legacyLoads==0 &&
        c.approvedDeletes==0 && c.portraitDeletes==0 && c.releaseCalls==1 &&
        c.CurFrame==0 && c.AnimationElapsedSeconds==0 && !c.IsActionEnd &&
        c.frameSetupCalls==1 && c.layoutCalls==1;
}
constexpr bool IncompletePair(int failure,bool originalFacing,bool haveLegacy)
{
    Character c;c.haveLegacy=haveLegacy;
    if(failure==0)c.haveApproved=false;
    if(failure==1)c.approved.bmWidth=1023;
    if(failure==2)c.approved.bmHeight=767;
    if(failure==3)c.havePortrait=false;
    if(failure==4)c.portraitImage.bmWidth=127;
    if(failure==5)c.portraitImage.bmHeight=127;
    const bool loaded=c.LoadMonsterSheet(L"Resource\\Monster\\",L"S1_Skeleton.bmp",originalFacing);
    if(loaded!=haveLegacy || c.bUseAuthoredMonsterLayout || c.hIllustration || c.legacyLoads!=1 ||
        c.approvedDeletes!=(failure==0?0:1) || c.portraitDeletes!=(failure>=4?1:0) ||
        c.portraitLoads!=(failure>=3?1:0) || c.MonsterImpactFrame!=1)return false;
    if(!haveLegacy)return !c.bUseMonsterSheetLayout && c.bSpriteFacesRight && c.frameSetupCalls==0 && c.layoutCalls==0;
    return c.bUseMonsterSheetLayout && c.bSpriteFacesRight==originalFacing && SharedAtlas(c,&c.legacy) &&
        c.SpriteInfoIdle.bmWidth==1536 && c.SpriteInfoDead.bmHeight==768 &&
        c.SpriteInfoIllustration.bmWidth==0 && c.SpriteInfoIllustration.bmHeight==0 &&
        c.frameSetupCalls==1 && c.layoutCalls==1;
}
constexpr bool AllFallbacks()
{
    for(int failure=0;failure<6;++failure)
        for(int facing=0;facing<2;++facing)
            for(int legacy=0;legacy<2;++legacy)
                if(!IncompletePair(failure,facing!=0,legacy!=0))return false;
    return true;
}
static_assert(CompletePair(false,false) && CompletePair(true,false) && CompletePair(false,true) && CompletePair(true,true),
    "Complete sets load matching atlas/portrait and canonical facing, including the bow angel's later contact pose");
static_assert(AllFallbacks(),"Every missing/malformed half-pair must clean up and retain the original whole-set fallback or fail cleanly");
'''
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(prefix + body + checks, encoding='utf-8')
    print('Prepared actual enemy loader fixture: complete pairs and 24 malformed/missing fallback cases.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'tests/bin/enemy-style-audit/enemy_loader_compile.cpp')
    generate(parser.parse_args().output.resolve())
