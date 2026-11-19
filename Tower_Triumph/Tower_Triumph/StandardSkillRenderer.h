#pragma once
#include "StandardSkillPresentation.h"
#include "UltimateSkillRenderer.h"

namespace StandardSkillRenderer
{
    namespace Model=StandardSkillPresentation;
    struct Options
    {
        HBITMAP bitmap=nullptr,projectile=nullptr,contact=nullptr;
        int tileSize=48,offsetX=0,offsetY=0;
        float footRatio=.75f;
        bool front=false;
    };
    inline void Sprite(HDC destination,HDC scratch,HBITMAP bitmap,int frame,int sourceCell,int columns,
        float coreX,float coreY,int size,float anchorX,float anchorY,float alpha,bool mirror=false)
    {
        if(!bitmap || frame<0 || !(alpha>0))return;
        const int saved=SaveDC(scratch);if(!saved)return;
        SelectObject(scratch,bitmap);
        HDC source=scratch,mirrorDC=nullptr;HBITMAP mirrorBitmap=nullptr;HGDIOBJ old=nullptr;
        int sx=(frame%columns)*sourceCell,sy=(frame/columns)*sourceCell;
        if(mirror)
        {
            mirrorDC=CreateCompatibleDC(destination);
            if(mirrorDC)mirrorBitmap=CreateCompatibleBitmap(destination,sourceCell,sourceCell);
            if(mirrorBitmap)
            {
                old=SelectObject(mirrorDC,mirrorBitmap);SetStretchBltMode(mirrorDC,COLORONCOLOR);
                if(StretchBlt(mirrorDC,sourceCell,0,-sourceCell,sourceCell,scratch,sx,sy,sourceCell,sourceCell,SRCCOPY))
                {source=mirrorDC;sx=0;sy=0;anchorX=sourceCell-anchorX;}
            }
        }
        const float scale=size/static_cast<float>(sourceCell);
        const int x=AreaSkillEffectRenderer::Detail::Round(coreX-anchorX*scale);
        const int y=AreaSkillEffectRenderer::Detail::Round(coreY-anchorY*scale);
        AreaSkillEffectRenderer::Detail::Surface surface(destination,{x,y,x+size,y+size});
        surface.Blend(alpha,[&](HDC dc){TransparentBlt(dc,x,y,size,size,source,sx,sy,sourceCell,sourceCell,RGB(255,0,255));});
        if(old && old!=HGDI_ERROR)SelectObject(mirrorDC,old);
        if(mirrorBitmap)DeleteObject(mirrorBitmap);if(mirrorDC)DeleteDC(mirrorDC);
        RestoreDC(scratch,saved);
    }
    inline void DrawTrail(HDC destination,const Model::Action& action,const Model::FlightSample& flight,const Options& options)
    {
        const auto motion=action.profile.motion;
        if(motion!=Model::Motion::VineWhip && motion!=Model::Motion::GrowingRay &&
            motion!=Model::Motion::PiercingArrow && motion!=Model::Motion::ForwardWave && motion!=Model::Motion::RadiantArrow)return;
        const float sx=options.offsetX+action.startX,sy=options.offsetY+action.startY;
        const float x=options.offsetX+flight.x,y=options.offsetY+flight.y;
        const auto palette=AreaSkillVisuals::Colors(action.profile.element);
        const RECT bounds{static_cast<LONG>((std::min)(sx,x))-12,static_cast<LONG>((std::min)(sy,y))-12,
            static_cast<LONG>((std::max)(sx,x))+13,static_cast<LONG>((std::max)(sy,y))+13};
        AreaSkillEffectRenderer::Detail::Surface surface(destination,bounds);
        surface.Blend(motion==Model::Motion::VineWhip?.7f:.55f,[&](HDC dc)
        {
            using AreaSkillEffectRenderer::Detail::Round;
            if(motion==Model::Motion::VineWhip)
            {
                POINT points[9]{};
                for(int i=0;i<9;++i){const auto p=Model::VinePoint(sx,sy,x,y,i);points[i]={Round(p.x),Round(p.y)};}
                AreaSkillEffectRenderer::Detail::Poly(dc,points,9,RGB(95,72,42),false,2);
                AreaSkillEffectRenderer::Detail::Poly(dc,points,9,AreaSkillEffectRenderer::Detail::Color(palette.accent),false,1);
            }
            else
            {
                const bool ray=motion==Model::Motion::GrowingRay;
                const float dx=x-sx,dy=y-sy,length=std::sqrt(dx*dx+dy*dy);
                const float tail=ray?1.0f:(length>0?(std::min)(1.0f,28.0f/length):0);
                const POINT points[2]={{Round(x-dx*tail),Round(y-dy*tail)},{Round(x),Round(y)}};
                AreaSkillEffectRenderer::Detail::Poly(dc,points,2,AreaSkillEffectRenderer::Detail::Color(palette.main),false,
                    motion==Model::Motion::ForwardWave?7:3);
                AreaSkillEffectRenderer::Detail::Poly(dc,points,2,AreaSkillEffectRenderer::Detail::Color(palette.bright),false,1);
            }
        });
    }
    inline void DrawFlight(HDC destination,HDC scratch,const Model::Action& action,float tx,float ty,float arrival,
        bool line,const Options& options)
    {
        const auto flight=Model::FlightAt(action,action.elapsed,tx,ty,arrival,line);if(!flight.visible)return;
        DrawTrail(destination,action,flight,options);
        const bool arrow=action.profile.owner==Model::Class::Archer;
        if(Model::Middle(action.profile) && options.bitmap)
        {
            const int frame=static_cast<int>((action.elapsed-action.release+BattleSkillArea::EventEpsilon)/.05f)%4;
            const auto anchor=Model::AuraAnchor(action.profile.element);
            Sprite(destination,scratch,options.bitmap,frame,128,4,options.offsetX+flight.x,options.offsetY+flight.y,
                Model::AuraSize(action.profile),anchor.x,anchor.y,arrow?.5f:.7f);
        }
        UltimateSkillRenderer::DrawDirectional(destination,scratch,options.projectile,64,flight.frame,Model::FlightSize(action.profile),
            options.offsetX+flight.x,options.offsetY+flight.y,flight.dx,flight.dy,
            32+(arrow?BattleActionTiming::ArrowTipOffsetX:BattleActionTiming::MagicCoreOffsetX),
            32+(arrow?BattleActionTiming::ArrowTipOffsetY:BattleActionTiming::MagicCoreOffsetY));
    }
    inline void DrawImpact(HDC destination,HDC scratch,const Model::Action& action,const BattleSkillArea::Cell& cell,
        const Options& options)
    {
        const auto sample=Model::ImpactAt(action.profile,action.elapsed,cell.arrivalSeconds);if(!sample.visible)return;
        const bool knight=Model::Knight(action.profile);
        if(!knight && !Model::Middle(action.profile))return;
        const bool center=cell.x==action.center.x && cell.y==action.center.y;
        const bool front=knight || (action.area!=BattleSkillArea::Kind::Single && action.area!=BattleSkillArea::Kind::Line && !center);
        if(front!=options.front)return;
        const float x=options.offsetX+(cell.x+.5f)*options.tileSize;
        const float foot=options.offsetY+(cell.y+options.footRatio)*options.tileSize;
        if(!options.bitmap)
        {
            // Valid target reactions supply the main contact; empty area cells still show a clean arrival.
            if(action.area!=BattleSkillArea::Kind::Single)
            {
                const int frame=(std::min)(7,static_cast<int>((action.elapsed-cell.arrivalSeconds)/.04f));
                Sprite(destination,scratch,options.contact,frame,64,4,x,foot-options.tileSize*.5f,40,32,32,sample.alpha);
            }
            return;
        }
        const auto anchor=Model::ImpactAnchor(action.profile);
        const int size=Model::CellSizeFor(action.profile,center,action.area);
        const float y=knight?foot-options.tileSize*.5f:foot;
        auto draw=[&](){Sprite(destination,scratch,options.bitmap,sample.frame,128,4,x,y,size,
            anchor.x,anchor.y,sample.alpha,Model::MirrorImpact(action.profile,action.facing));};
        if(knight){draw();return;}
        const float scale=size/128.0f;
        const int left=AreaSkillEffectRenderer::Detail::Round(x-anchor.x*scale);
        const int top=AreaSkillEffectRenderer::Detail::Round(y-anchor.y*scale);
        const int split=top+AreaSkillEffectRenderer::Detail::Round(anchor.split*scale);
        int saved=SaveDC(destination);
        if(saved){IntersectClipRect(destination,left,top,left+size,split);draw();RestoreDC(destination,saved);}
        HRGN reached=CreateRectRgn(0,0,0,0);if(!reached)return;
        for(const auto& reachedCell:action.cells)
        {
            if(!BattleSkillArea::IsDue(action.elapsed,reachedCell.arrivalSeconds))continue;
            const int cx=options.offsetX+reachedCell.x*options.tileSize,cy=options.offsetY+reachedCell.y*options.tileSize;
            HRGN rect=CreateRectRgn(cx,cy,cx+options.tileSize,cy+options.tileSize);
            if(rect){CombineRgn(reached,reached,rect,RGN_OR);DeleteObject(rect);}
        }
        saved=SaveDC(destination);
        if(saved)
        {
            if(ExtSelectClipRgn(destination,reached,RGN_AND)!=ERROR){IntersectClipRect(destination,left,split,left+size,top+size);draw();}
            RestoreDC(destination,saved);
        }
        DeleteObject(reached);
    }
    inline void Draw(HDC destination,HDC scratch,const Model::Action& action,const Options& options)
    {
        if(!destination || !scratch || destination==scratch || !Model::Valid(action.profile))return;
        if(options.front && !Model::Knight(action.profile))
        {
            if(Model::Fan(action.profile))
                for(const auto& cell:action.cells)
                    DrawFlight(destination,scratch,action,(cell.x+.5f)*options.tileSize,
                        (cell.y+options.footRatio-.5f)*options.tileSize,cell.arrivalSeconds,false,options);
            else DrawFlight(destination,scratch,action,action.targetX,action.targetY,action.arrival,
                action.area==BattleSkillArea::Kind::Line,options);
        }
        for(const auto& cell:action.cells)DrawImpact(destination,scratch,action,cell,options);
    }
}
