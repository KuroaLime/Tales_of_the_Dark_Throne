#include "../Tower_Triumph/Tower_Triumph/AreaSkillVisuals.h"

namespace Visual = AreaSkillVisuals;
constexpr bool ValidSamples()
{
    for(int element=1;element<=5;++element)
        for(int x=0;x<4;++x)
            for(int frame=-2;frame<=70;++frame)
            {
                const auto kind=static_cast<Visual::Element>(element);
                const float age=frame*.01f;
                const auto cell=Visual::SampleCell(kind,age,1.22f,.3f);
                if(cell.glowAlpha<0 || cell.glowAlpha>1 || cell.plumeAlpha<0 || cell.plumeAlpha>1)return false;
                if(age<0 || age>=Visual::Lifetime)
                    if(cell.glowAlpha!=0 || cell.plumeAlpha!=0)return false;
                for(int i=0;i<Visual::ParticleCount(kind);++i)
                {
                    const auto p=Visual::SampleParticle(kind,x,6,i,age,1.22f);
                    const auto again=Visual::SampleParticle(kind,x,6,i,age,1.22f);
                    if(p.x!=again.x || p.y!=again.y || p.alpha!=again.alpha || p.alpha<0 || p.alpha>1)return false;
                    if((age<0 || age>=Visual::Lifetime) && p.alpha!=0)return false;
                    if(p.front && p.alpha>0 && p.y<-.28001f)return false;
                    if(Visual::Band(p.alpha)<0 || Visual::Band(p.alpha)>Visual::AlphaBands)return false;
                }
            }
    return true;
}
static_assert(ValidSamples(), "All five elements must be deterministic, bounded, absent before impact and fully expired");
static_assert(Visual::Lifetime==.62f && Visual::BodyDrawSize==64 && Visual::BodySeconds>.33f && Visual::BodySeconds<.35f,
    "Vivid effects extend only presentation lifetime and AoE contact size");
static_assert(Visual::BurstFrame(-.01f)==-1 && Visual::BurstFrame(0)==0 && Visual::BurstFrame(.28f)==7 &&
    Visual::BurstFrame(.33f)==-1 && Visual::BodyFrame(.29f)==7 && Visual::BodyAlpha(.34f)==0,
    "Authored source frames remain in range and afterglow expires without a frozen sprite");
static_assert(Visual::CenterStrength(BattleSkillArea::Kind::Splash2,4,5,4,5)==1.22f &&
    Visual::CenterStrength(BattleSkillArea::Kind::Splash2,4,6,4,5)==1.0f &&
    Visual::CenterStrength(BattleSkillArea::Kind::Line,4,5,4,5)==1.0f,
    "Only the explosion center receives a visual boost; line cells keep consistent strength");
static_assert(Visual::SampleCell(Visual::Element::Light,.1f,1,.1f).plumeHeight <
    Visual::SampleCell(Visual::Element::Light,.1f,1,.9f).plumeHeight &&
    Visual::Seed(3,4,1)!=Visual::Seed(4,4,1), "Cell seeds vary height and particles without changing arrival time");
static_assert(Visual::SampleCell(Visual::Element::Fire,.08f,1.22f).glowWidth < 1.0f,
    "The boosted central ground glow must fit inside its cell without a square clipped edge");
constexpr bool LightRays()
{
    for(int x=0;x<4;++x)
        for(int frame=0;frame<64;++frame)
        {
            const float age=frame*.01f;
            float previousAlpha=2.0f;
            for(int segment=0;segment<Visual::LightRaySegments;++segment)
            {
                const auto ray=Visual::SampleLightRay(x,6,age,1.22f,segment);
                if(ray.alpha<0 || ray.alpha>1 || ray.alpha>previousAlpha)return false;
                if(age<Visual::Lifetime && (ray.width<1 || ray.width>2))return false;
                if(age>=.44f && ray.alpha!=0)return false;
                if(segment>0)
                {
                    const auto before=Visual::SampleLightRay(x,6,age,1.22f,segment-1);
                    if(before.x1!=ray.x0 || before.y1!=ray.y0)return false;
                }
                previousAlpha=ray.alpha;
            }
        }
    return Visual::SampleLightRay(2,6,-.01f,1,0).alpha==0 &&
        Visual::SampleLightRay(2,6,.1f,1,3).alpha < Visual::SampleLightRay(2,6,.1f,1,0).alpha;
}
static_assert(LightRays(), "Thin seeded rays must connect, taper in opacity, stay 1-2 pixels wide and expire");
static_assert(Visual::SampleCell(Visual::Element::Light,.1f).ringAlpha ==
    Visual::SampleCell(Visual::Element::Fire,.1f).ringAlpha*.5f &&
    Visual::SampleCell(Visual::Element::Light,.1f).spriteAlpha ==
    Visual::SampleCell(Visual::Element::Fire,.1f).spriteAlpha*.75f,
    "Only Light lowers the floor ring and floor sprite; other effects retain the vivid contract");
