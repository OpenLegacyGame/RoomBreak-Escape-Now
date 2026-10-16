#pragma once
namespace rb
{
inline const char* shopFreeText(int language)
{
    static const char* text[] = {"免费", "무료", "FREE", "無料"};
    return text[language];
}
inline const char* shopUnlockText(int language)
{
    static const char* text[] = {"解锁全部关卡", "모든 스테이지 잠금 해제", "Unlock all stages", "全ステージを解放"};
    return text[language];
}
inline const char* shopUnlockConfirm(int language)
{
    static const char* text[] = {"确定解锁全部章节和关卡？", "모든 에피소드와 스테이지를\n해제할까요?",
                                 "Unlock every episode\nand stage?", "全エピソードとステージを解放しますか？"};
    return text[language];
}
}  // namespace rb
