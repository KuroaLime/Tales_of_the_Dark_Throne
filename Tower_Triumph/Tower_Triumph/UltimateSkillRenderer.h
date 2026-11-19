#pragma once
#include "SkillPresentation.h"
#include "AreaSkillEffectRenderer.h"

namespace UltimateSkillRenderer
{
    struct Options
    {
        SkillPresentation::Kind kind=SkillPresentation::Kind::Normal;
        BattleActionTiming::ElementVisual element=BattleActionTiming::ElementVisual::Neutral;
        HBITMAP bitmap=nullptr,flightBitmap=nullptr,rainLaunchBitmap=nullptr,contactBitmap=nullptr;
        int centerX=0,centerY=0,tileSize=48,offsetX=0,offsetY=0;
        float footRatio=.75f,deliverySeconds=-1,facing=1;
        float casterFootX=0,casterFootY=0,startX=0,startY=0,targetX=0,targetY=0;
        bool front=false;
    };
    inline void DrawCharge(HDC destination,float age,const Options& options)
    {
        const float t=SkillPresentation::AreaClamp((age+SkillPresentation::WindupSeconds)/SkillPresentation::WindupSeconds);
        const float x=options.startX,y=options.startY;
        const RECT bounds{static_cast<LONG>(x)-24,static_cast<LONG>(y)-24,static_cast<LONG>(x)+24,static_cast<LONG>(y)+24};
        AreaSkillEffectRenderer::Detail::Surface surface(destination,bounds);
        const auto palette=AreaSkillVisuals::Colors(options.element);
        surface.Blend(.20f+.40f*t,[&](HDC dc)
        {
            const float radius=7.0f+8.0f*t;
            AreaSkillEffectRenderer::Detail::EllipseShape(dc,x,y,radius*2,radius*2,
                AreaSkillEffectRenderer::Detail::Color(palette.main),true);
            AreaSkillEffectRenderer::Detail::EllipseShape(dc,x,y,5.0f+6*t,5.0f+6*t,
                AreaSkillEffectRenderer::Detail::Color(palette.bright),true);
        });
    }
    inline void DrawDirectional(HDC destination,HDC scratch,HBITMAP bitmap,int sourceCell,int frame,int drawSize,
        float x,float y,float dx,float dy,float anchorX,float anchorY)
    {
        if(!bitmap)return;
        const int direction=BattleActionTiming::DirectionFromRadians(atan2f(dy,dx));
        const float cosine=BattleActionTiming::DirectionCosines[direction];
        const float sine=BattleActionTiming::DirectionCosines[(direction+12)%16];
        const float center=sourceCell*.5f,scale=drawSize/static_cast<float>(sourceCell);
        const float ax=anchorX-center,ay=anchorY-center;
        const int left=AreaSkillEffectRenderer::Detail::Round(x-(ax*cosine-ay*sine)*scale-drawSize*.5f);
        const int top=AreaSkillEffectRenderer::Detail::Round(y-(ax*sine+ay*cosine)*scale-drawSize*.5f);
        const RECT bounds{left,top,left+drawSize,top+drawSize};
        const int saved=SaveDC(scratch);if(!saved)return;
        SelectObject(scratch,bitmap);
        AreaSkillEffectRenderer::Detail::Surface surface(destination,bounds);
        surface.Blend(1.0f,[&](HDC dc)
        {TransparentBlt(dc,left,top,drawSize,drawSize,scratch,direction*sourceCell,frame*sourceCell,
            sourceCell,sourceCell,RGB(255,0,255));});
        RestoreDC(scratch,saved);
    }
    inline void DrawRainArrows(HDC destination,HDC scratch,const std::vector<BattleSkillArea::Cell>& cells,
        float commandElapsed,const Options& options)
    {
        if(!options.front || !SkillPresentation::IsRain(options.kind) || !options.rainLaunchBitmap)return;
        const float scale=options.tileSize/48.0f;
        for(const auto& cell:cells)
        {
            const auto arrow=SkillPresentation::RainArrowAt(commandElapsed,cell.arrivalSeconds);
            if(!arrow.visible)continue;
            const float x=options.offsetX+(cell.x+.5f)*options.tileSize;
            const float y=options.offsetY+(cell.y+options.footRatio-.5f)*options.tileSize;
            DrawDirectional(destination,scratch,options.rainLaunchBitmap,BattleActionTiming::ProjectileCellSize,arrow.frame,
                SkillPresentation::RainArrowSize,x+arrow.offsetX*scale,y+arrow.offsetY*scale,
                -SkillPresentation::RainArrowStartX,-SkillPresentation::RainArrowStartY,
                32.0f+BattleActionTiming::ArrowTipOffsetX,32.0f+BattleActionTiming::ArrowTipOffsetY);
        }
    }
    inline void DrawCellSummonShape(HDC dc,float x,float y,const SkillPresentation::CellSummonSample& state,
        float scale,const Options& options)
    {
        using AreaSkillEffectRenderer::Detail::Round;
        using AreaSkillEffectRenderer::Detail::Poly;
        const auto palette=AreaSkillVisuals::Colors(options.element);
        const float h=state.height*scale,lean=state.lean*scale;
        if(options.kind==SkillPresentation::Kind::TidalBlast)
        {
            for(int side:{-1,1})
            {
                const float base=x+side*7*scale,tip=x+lean+side*13*scale;
                const POINT jet[7]={{Round(base-5*scale),Round(y)},
                    {Round(base-7*scale),Round(y-h*.28f)},{Round(tip-5*scale),Round(y-h*.78f)},
                    {Round(tip+side*5*scale),Round(y-h)}, {Round(tip+4*scale),Round(y-h*.70f)},
                    {Round(base+5*scale),Round(y-h*.20f)},{Round(base+4*scale),Round(y)}};
                Poly(dc,jet,7,AreaSkillEffectRenderer::Detail::Color(palette.accent),true);
            }
        }
        else if(options.kind==SkillPresentation::Kind::RootStorm)
        {
            for(int side:{-1,1})
            {
                POINT stem[SkillPresentation::RootStemSegments+1]{};
                for(int point=0;point<=SkillPresentation::RootStemSegments;++point)
                {
                    const auto p=SkillPresentation::RootStemPoint(state,side,point);
                    stem[point]={Round(x+p.x*scale),Round(y+p.y*scale)};
                }
                for(int segment=0;segment<SkillPresentation::RootStemSegments;++segment)
                    Poly(dc,stem+segment,2,RGB(95,72,42),false,SkillPresentation::RootStemWidth(segment));
                Poly(dc,stem,SkillPresentation::RootStemSegments+1,AreaSkillEffectRenderer::Detail::Color(palette.accent),false,1);
                POINT branch[3]{};
                for(int point=0;point<3;++point)
                {
                    const auto p=SkillPresentation::RootBranchPoint(state,side,point);
                    branch[point]={Round(x+p.x*scale),Round(y+p.y*scale)};
                }
                Poly(dc,branch,3,RGB(95,72,42),false,1);
                const auto leaf=SkillPresentation::RootBranchPoint(state,side,2);
                const float lx=x+leaf.x*scale,ly=y+leaf.y*scale;
                const POINT points[4]={{Round(lx),Round(ly)},{Round(lx+side*1.5f*scale),Round(ly-2*scale)},
                    {Round(lx+side*3*scale),Round(ly-2*scale)},{Round(lx+side*1.5f*scale),Round(ly)}};
                Poly(dc,points,4,AreaSkillEffectRenderer::Detail::Color(palette.accent),true);
            }
        }
        else
        {
            for(int side:{-1,1})
            {
                const POINT crack[5]={{Round(x+side*19*scale),Round(y-scale)},
                    {Round(x+side*9*scale),Round(y-4*scale)}, {Round(x+side*5*scale),Round(y-h*.28f)},
                    {Round(x+lean+side*9*scale),Round(y-h*.54f)}, {Round(x+lean),Round(y-h*.82f)}};
                Poly(dc,crack,5,AreaSkillEffectRenderer::Detail::Color(palette.accent),false,2);
            }
            AreaSkillEffectRenderer::Detail::EllipseShape(dc,x+lean,y-h*.35f,state.width*scale*.75f,h*.35f,
                AreaSkillEffectRenderer::Detail::Color(palette.bright),false,1);
        }
    }
    inline void DrawCellSummons(HDC destination,HDC scratch,const std::vector<BattleSkillArea::Cell>& cells,
        float commandElapsed,const Options& options)
    {
        if(!options.front || !SkillPresentation::IsCellSummon(options.kind))return;
        RECT bounds{};bool first=true;unsigned int bands=0;
        const float scale=options.tileSize/48.0f;
        for(const auto& cell:cells)
        {
            const auto state=SkillPresentation::CellSummonAt(options.kind,cell.x,cell.y,commandElapsed,cell.arrivalSeconds);
            if(!state.visible)continue;
            const int x=options.offsetX+cell.x*options.tileSize,y=options.offsetY+cell.y*options.tileSize;
            const RECT cellBounds{x-options.tileSize,y-options.tileSize*2,x+options.tileSize*2,y+options.tileSize};
            if(first){bounds=cellBounds;first=false;}else UnionRect(&bounds,&bounds,&cellBounds);
            bands|=AreaSkillEffectRenderer::Detail::BandBit(state.shapeAlpha);
            if(options.contactBitmap)bands|=AreaSkillEffectRenderer::Detail::BandBit(state.alpha);
        }
        if(first || !bands)return;
        const int saved=SaveDC(scratch);
        if(saved && options.contactBitmap)SelectObject(scratch,options.contactBitmap);
        AreaSkillEffectRenderer::Detail::Surface surface(destination,bounds);
        for(int band=1;band<=AreaSkillVisuals::AlphaBands;++band)
        {
            if(!(bands&(1u<<band)))continue;
            surface.Blend(band/static_cast<float>(AreaSkillVisuals::AlphaBands),[&](HDC dc)
            {
                for(const auto& cell:cells)
                {
                    const auto state=SkillPresentation::CellSummonAt(options.kind,cell.x,cell.y,commandElapsed,cell.arrivalSeconds);
                    if(!state.visible)continue;
                    const float x=options.offsetX+(cell.x+.5f)*options.tileSize;
                    const float y=options.offsetY+(cell.y+options.footRatio)*options.tileSize;
                    if(AreaSkillVisuals::Band(state.shapeAlpha)==band)DrawCellSummonShape(dc,x,y,state,scale,options);
                    if(saved && options.contactBitmap && AreaSkillVisuals::Band(state.alpha)==band)
                    {
                        const float width=state.width*scale,height=state.height*scale;
                        TransparentBlt(dc,AreaSkillEffectRenderer::Detail::Round(x-width*.5f),
                            AreaSkillEffectRenderer::Detail::Round(y-height),
                            AreaSkillEffectRenderer::Detail::Round(width),AreaSkillEffectRenderer::Detail::Round(height),
                            scratch,(state.frame%4)*64,(state.frame/4)*64,64,64,RGB(255,0,255));
                    }
                }
            });
        }
        if(saved)RestoreDC(scratch,saved);
    }
    inline void Draw(HDC destination,HDC scratch,const std::vector<BattleSkillArea::Cell>& cells,
        float commandElapsed,float releaseSeconds,const Options& options)
    {
        if(!destination || !scratch || destination==scratch || !options.bitmap)return;
        const float age=commandElapsed-releaseSeconds;
        const auto sample=SkillPresentation::At(options.kind,age,options.deliverySeconds);
        if(sample.phase==SkillPresentation::Phase::Hidden)return;
        if(SkillPresentation::Front(options.kind,sample.phase)!=options.front)
        {
            // Outer-ring arrows remain in front after the central landing has moved to the back pass.
            DrawRainArrows(destination,scratch,cells,commandElapsed,options);
            DrawCellSummons(destination,scratch,cells,commandElapsed,options);
            return;
        }
        const bool knight=SkillPresentation::IsKnight(options.kind),snipe=SkillPresentation::IsSnipe(options.kind);
        const bool rain=SkillPresentation::IsRain(options.kind);
        if(sample.phase==SkillPresentation::Phase::Charge && !knight){DrawCharge(destination,age,options);return;}
        const auto anchors=SkillPresentation::Anchors(options.kind);
        if(snipe && sample.phase==SkillPresentation::Phase::Flight)
        {
            const float t=SkillPresentation::AreaClamp(age/options.deliverySeconds);
            const float dx=options.targetX-options.startX,dy=options.targetY-options.startY;
            DrawDirectional(destination,scratch,options.flightBitmap,SkillPresentation::CellSize,sample.frame,
                SkillPresentation::SnipeDrawSize,options.startX+dx*t,options.startY+dy*t,dx,dy,
                static_cast<float>(anchors.coreX),static_cast<float>(anchors.coreY));
            return;
        }
        if(rain && sample.phase==SkillPresentation::Phase::Flight && age<SkillPresentation::RainRiseSeconds)
        {
            const float t=SkillPresentation::AreaClamp(age/SkillPresentation::RainRiseSeconds);
            const float dx=options.facing*24.0f,dy=-160.0f;
            const int frame=static_cast<int>((age+BattleSkillArea::EventEpsilon)/BattleActionTiming::ProjectileFrameSeconds)%4;
            DrawDirectional(destination,scratch,options.rainLaunchBitmap,BattleActionTiming::ProjectileCellSize,frame,
                SkillPresentation::RainLaunchSize,options.startX+dx*t,options.startY+dy*t,dx,dy,
                32.0f+BattleActionTiming::ArrowTipOffsetX,32.0f+BattleActionTiming::ArrowTipOffsetY);
            return;
        }
        const bool falling=sample.phase==SkillPresentation::Phase::Flight && (options.kind==SkillPresentation::Kind::Meteor || rain);
        int size=falling && !rain?SkillPresentation::MeteorDrawSize:SkillPresentation::MainDrawSizeFor(options.kind);
        if(knight)size=sample.phase==SkillPresentation::Phase::Charge?SkillPresentation::KnightChargeSize:SkillPresentation::KnightImpactSize;
        if(snipe)size=SkillPresentation::SnipeImpactSize;
        const float scale=size/static_cast<float>(SkillPresentation::CellSize);
        float anchorX=falling?anchors.coreX:static_cast<float>(anchors.groundX);
        float anchorY=falling?anchors.coreY:static_cast<float>(anchors.groundY);
        float coreX=options.centerX+(falling?sample.coreX:0.0f),coreY=options.centerY+(falling?sample.coreY:0.0f);
        if(knight && sample.phase==SkillPresentation::Phase::Charge)
        {
            anchorX=static_cast<float>(anchors.chargeX);anchorY=static_cast<float>(anchors.chargeY);
            coreX=options.casterFootX+options.facing*sample.coreX;coreY=options.casterFootY+sample.coreY;
        }
        if(options.kind==SkillPresentation::Kind::KnightWater && sample.phase==SkillPresentation::Phase::Impact && !cells.empty())
        {
            const float t=SkillPresentation::LineWaveProgress(age,static_cast<int>(cells.size()));
            coreX=options.offsetX+(cells.front().x+.5f+(cells.back().x-cells.front().x)*t)*options.tileSize;
            coreY=options.offsetY+(cells.front().y+options.footRatio+(cells.back().y-cells.front().y)*t)*options.tileSize;
        }
        if(snipe){coreX=options.targetX;coreY=options.targetY;}
        const int savedScratch=SaveDC(scratch);if(!savedScratch)return;
        SelectObject(scratch,options.bitmap);
        HDC source=scratch,mirrorDC=nullptr;HBITMAP mirrorBitmap=nullptr;HGDIOBJ previousMirror=nullptr;
        int sourceX=(sample.frame%4)*128,sourceY=(sample.frame/4)*128;
        if(knight && options.facing<0)
        {
            mirrorDC=CreateCompatibleDC(destination);
            if(mirrorDC)mirrorBitmap=CreateCompatibleBitmap(destination,128,128);
            if(mirrorBitmap)
            {
                previousMirror=SelectObject(mirrorDC,mirrorBitmap);
                SetStretchBltMode(mirrorDC,COLORONCOLOR);
                if(StretchBlt(mirrorDC,128,0,-128,128,scratch,sourceX,sourceY,128,128,SRCCOPY))
                {source=mirrorDC;sourceX=0;sourceY=0;anchorX=128-anchorX;}
            }
        }
        const int x=AreaSkillEffectRenderer::Detail::Round(coreX-anchorX*scale);
        const int y=AreaSkillEffectRenderer::Detail::Round(coreY-anchorY*scale);
        const RECT bounds{x,y,x+size,y+size};
        auto sprite=[&](HDC dc)
        {TransparentBlt(dc,x,y,size,size,source,sourceX,sourceY,128,128,RGB(255,0,255));};
        if(falling || snipe || (knight && sample.phase==SkillPresentation::Phase::Charge))
        {
            AreaSkillEffectRenderer::Detail::Surface surface(destination,bounds);surface.Blend(sample.opacity,sprite);
        }
        else
        {
            const int split=y+AreaSkillEffectRenderer::Detail::Round(anchors.groundStartY*scale);
            int saved=SaveDC(destination);
            if(saved)
            {
                IntersectClipRect(destination,x,y,x+size,split);
                AreaSkillEffectRenderer::Detail::Surface surface(destination,bounds);surface.Blend(sample.opacity,sprite);
                RestoreDC(destination,saved);
            }
            if(sample.phase==SkillPresentation::Phase::Impact || SkillPresentation::IsCellSummon(options.kind))
            {
                HRGN reached=CreateRectRgn(0,0,0,0);
                if(reached)
                {
                    for(const auto& cell:cells)
                    {
                        if(sample.phase==SkillPresentation::Phase::Impact && !BattleSkillArea::IsDue(commandElapsed,cell.arrivalSeconds))continue;
                        const int cx=options.offsetX+cell.x*options.tileSize,cy=options.offsetY+cell.y*options.tileSize;
                        HRGN rectangle=CreateRectRgn(cx,cy,cx+options.tileSize,cy+options.tileSize);
                        if(rectangle){CombineRgn(reached,reached,rectangle,RGN_OR);DeleteObject(rectangle);}
                    }
                    saved=SaveDC(destination);
                    if(saved)
                    {
                        if(ExtSelectClipRgn(destination,reached,RGN_AND)!=ERROR)
                        {
                            IntersectClipRect(destination,x,split,x+size,y+size);
                            AreaSkillEffectRenderer::Detail::Surface surface(destination,bounds);surface.Blend(sample.opacity,sprite);
                        }
                        RestoreDC(destination,saved);
                    }
                    DeleteObject(reached);
                }
            }
        }
        if(previousMirror && previousMirror!=HGDI_ERROR)SelectObject(mirrorDC,previousMirror);
        if(mirrorBitmap)DeleteObject(mirrorBitmap);if(mirrorDC)DeleteDC(mirrorDC);
        RestoreDC(scratch,savedScratch);
        // Keep the individual arrival arrows visible above the large airborne volley.
        DrawRainArrows(destination,scratch,cells,commandElapsed,options);
        DrawCellSummons(destination,scratch,cells,commandElapsed,options);
    }
}
