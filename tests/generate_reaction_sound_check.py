"""Extract production state/damage bodies for compile-only sound-policy checks.

Only the audio dependency becomes an instance recorder so constexpr evaluations
can observe it. The state, HP, death, and animation statements are unchanged.
This generator never starts a native executable or a build.
"""
import argparse
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Tower_Triumph" / "Tower_Triumph"


def definition(text, method):
    start = text.index("void Character::" + method + "(")
    opening = text.index("{", start)
    depth = 1
    cursor = opening + 1
    while depth:
        depth += (text[cursor] == "{") - (text[cursor] == "}")
        cursor += 1
    return text[start:cursor]


def generate(output):
    source = (SOURCE / "Character.cpp").read_text(encoding="utf-8-sig")
    header = (SOURCE / "Character.h").read_text(encoding="utf-8-sig")
    declarations = []
    bodies = []
    for name in ("SetState", "GetDamage", "Die"):
        match = re.search(r"\bvoid " + name + r"\([^;]+\);", header)
        assert match and " = true" in match[0], name + " must preserve audible defaults"
        declarations.append("constexpr " + match[0])
        body = definition(source, name)
        if name == "SetState":
            assert body.index("if (!playSound) return;") > body.index("UpdateMaxFrameByState();")
            assert body.count("SoundManager::PlaySFX") == 5
            body = body.replace("SoundManager::PlaySFX", "sound.PlaySFX")
        bodies.append("constexpr " + body)

    prefix = r'''
#include <string_view>
enum class ECharacterState { Idle, Walk, Attack, Skill, Defend, Hit, Dead };
struct SoundRecorder
{
    int calls=0;
    const wchar_t* last=L"";
    constexpr void PlaySFX(const wchar_t* file){++calls;last=file;}
};
struct Character
{
    ECharacterState State=ECharacterState::Idle;
    struct Stats {int CurHP=30;} Stat;
    struct Recovery {int value=7;} IdleRecovery;
    int ActionSequence=4,CurFrame=5,layoutUpdates=0,recoveryCalls=0;
    float WalkCycleDistance=13,AnimationElapsedSeconds=.4f,DefendRate=.5f;
    bool KeepWalkingOnArrival=true,IsActionEnd=true,IsMoving=true;
    SoundRecorder sound;
    constexpr void BeginIdleRecovery(ECharacterState){++recoveryCalls;IdleRecovery.value=9;}
    constexpr void UpdateMaxFrameByState(){++layoutUpdates;}
'''
    checks = r'''
constexpr bool SameGameplay(const Character& a,const Character& b)
{
    return a.State==b.State && a.Stat.CurHP==b.Stat.CurHP && a.ActionSequence==b.ActionSequence &&
        a.CurFrame==b.CurFrame && a.AnimationElapsedSeconds==b.AnimationElapsedSeconds &&
        a.WalkCycleDistance==b.WalkCycleDistance && a.KeepWalkingOnArrival==b.KeepWalkingOnArrival &&
        a.IsActionEnd==b.IsActionEnd && a.IsMoving==b.IsMoving &&
        a.IdleRecovery.value==b.IdleRecovery.value && a.layoutUpdates==b.layoutUpdates &&
        a.recoveryCalls==b.recoveryCalls;
}
constexpr bool DamageCase(ECharacterState before,int damage,int expectedHP,ECharacterState after)
{
    Character audible,silent;audible.State=before;silent.State=before;
    audible.GetDamage(damage);silent.GetDamage(damage,false);
    const bool alreadyDead=before==ECharacterState::Dead;
    const auto expectedSound=after==ECharacterState::Dead?L"Resource\\Sound\\Character_Dead.wav":L"Resource\\Sound\\Character_Hit.wav";
    return SameGameplay(audible,silent) && audible.Stat.CurHP==expectedHP && audible.State==after &&
        audible.sound.calls==(alreadyDead?0:1) && silent.sound.calls==0 &&
        (alreadyDead || (std::wstring_view(audible.sound.last)==expectedSound && audible.CurFrame==0 &&
            audible.AnimationElapsedSeconds==0 && !audible.IsActionEnd && audible.ActionSequence==5));
}
constexpr bool DeathCase()
{
    Character audible,silent;audible.Die();silent.Die(false);
    return SameGameplay(audible,silent) && audible.Stat.CurHP==0 && !audible.IsMoving &&
        !audible.KeepWalkingOnArrival && audible.State==ECharacterState::Dead &&
        audible.sound.calls==1 && silent.sound.calls==0;
}
constexpr bool StateCase(ECharacterState state,bool restart)
{
    Character audible,silent;audible.SetState(state,restart);silent.SetState(state,restart,false);
    const int sounds=(state==ECharacterState::Idle || state==ECharacterState::Walk)?0:1;
    return SameGameplay(audible,silent) && audible.sound.calls==sounds && silent.sound.calls==0;
}
constexpr bool RecoveryAndNoRestart()
{
    Character audible,silent;audible.State=ECharacterState::Skill;silent.State=ECharacterState::Skill;
    audible.SetState(ECharacterState::Idle);silent.SetState(ECharacterState::Idle,false,false);
    if(!SameGameplay(audible,silent) || audible.recoveryCalls!=1)return false;
    Character untouched;untouched.State=ECharacterState::Hit;untouched.SetState(ECharacterState::Hit);
    return untouched.CurFrame==5 && untouched.ActionSequence==4 && untouched.sound.calls==0;
}
static_assert(DamageCase(ECharacterState::Idle,10,20,ECharacterState::Hit),"ordinary/terrain sound preserved; skill hit muted");
static_assert(DamageCase(ECharacterState::Defend,10,25,ECharacterState::Hit),"defense damage is unchanged");
static_assert(DamageCase(ECharacterState::Hit,10,20,ECharacterState::Hit),"repeated hit still restarts animation");
static_assert(DamageCase(ECharacterState::Idle,50,0,ECharacterState::Dead),"lethal skill propagates silence through Die");
static_assert(DamageCase(ECharacterState::Idle,-3,30,ECharacterState::Hit),"negative damage handling unchanged");
static_assert(DamageCase(ECharacterState::Dead,10,30,ECharacterState::Dead),"dead guard unchanged");
static_assert(DeathCase(),"natural death remains audible by default");
static_assert(StateCase(ECharacterState::Attack,true),"default attack sound and animation");
static_assert(StateCase(ECharacterState::Skill,true),"default skill sound and animation");
static_assert(StateCase(ECharacterState::Defend,true),"default guard sound and animation");
static_assert(StateCase(ECharacterState::Hit,true),"default hit sound and animation");
static_assert(StateCase(ECharacterState::Dead,true),"default death sound and animation");
static_assert(StateCase(ECharacterState::Walk,false),"walking unchanged");
static_assert(RecoveryAndNoRestart(),"idle recovery and same-state behavior unchanged");
'''
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(prefix + "\n".join(declarations) + "\n};\n" +
                      "\n".join(bodies) + checks, encoding="utf-8")
    print(f"Prepared actual-body reaction sound fixture (14 static_assert checks): {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "tests/bin/reaction_sound_compile.cpp")
    generate(parser.parse_args().output.resolve())
