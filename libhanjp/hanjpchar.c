#include "hanjpchar.h"
#include "hanjp.h"

#define HALF_KATAKANA_VOICED_MARK 0xFF9E
#define HALF_KATAKANA_SEMI_VOICED_MARK 0xFF9F
#define HALF_KATAKANA_SMALL_A 0xFF67
#define HALF_KATAKANA_SMALL_YA 0xFF6C
#define HALF_KATAKANA_SMALL_YU 0xFF6D
#define HALF_KATAKANA_SMALL_YO 0xFF6E
#define HALF_KATAKANA_SMALL_TSU 0xFF6D

static const ucschar kana_table[][5][3] = {
    // {A(0), I(1), U(2), E(3), O(4)}
    {{0x3042, 0x30A2, 0xFF71}, {0x3044, 0x30A4, 0xFF72}, {0x3046, 0x30A6, 0xFF73}, {0x3048, 0x30A8, 0xFF74}, {0x304A, 0x30AA, 0xFF75}}, // O(0)
    {{0x304B, 0x30AB, 0xFF76}, {0x304D, 0x30AD, 0xFF77}, {0x304F, 0x30AF, 0xFF78}, {0x3051, 0x30B1, 0xFF79}, {0x3053, 0x30B3, 0xFF7A}}, // K(1)
    {{0x3055, 0x30B5, 0xFF7B}, {0x3057, 0x30B7, 0xFF7C}, {0x3059, 0x30B9, 0xFF7D}, {0x305B, 0x30BB, 0xFF7E}, {0x305D, 0x30BD, 0xFF7F}}, // S(2)
    {{0x305F, 0x30BF, 0xFF80}, {0x3061, 0x30C1, 0xFF81}, {0x3064, 0x30C4, 0xFF82}, {0x3066, 0x30C6, 0xFF83}, {0x3068, 0x30C8, 0xFF84}}, // T(3)
    {{0x306A, 0x30CA, 0xFF85}, {0x306B, 0x30CB, 0xFF86}, {0x306C, 0x30CC, 0xFF87}, {0x306D, 0x30CD, 0xFF88}, {0x306E, 0x30CE, 0xFF89}}, // N(4)
    {{0x306F, 0x30CF, 0xFF8A}, {0x3072, 0x30D2, 0xFF8B}, {0x3075, 0x30D5, 0xFF8C}, {0x3078, 0x30D8, 0xFF8D}, {0x307B, 0x30DB, 0xFF8E}}, // H(5)
    {{0x307E, 0x30DE, 0xFF8F}, {0x307F, 0x30DF, 0xFF90}, {0x3080, 0x30E0, 0xFF91}, {0x3081, 0x30E1, 0xFF92}, {0x3082, 0x30E2, 0xFF93}}, // M(6)
    {{0x3084, 0x30E4, 0xFF94}, {0x0000, 0x0000, 0x0000}, {0x3086, 0x30E6, 0xFF95}, {0x0000, 0x0000, 0x0000}, {0x3088, 0x30E8, 0xFF96}}, // Y(7)
    {{0x3089, 0x30E9, 0xFF97}, {0x308A, 0x30EA, 0xFF98}, {0x308B, 0x30EB, 0xFF99}, {0x308C, 0x30EC, 0xFF9A}, {0x308D, 0x30ED, 0xFF9B}}, // R(8)
    {{0x308F, 0x30EF, 0xFF9C}, {0x3090, 0x30F0, 0x0000}, {0x0000, 0x0000, 0x0000}, {0x3091, 0x30F1, 0x0000}, {0x3092, 0x30F2, 0xFF66}}, // W(9)
    {{0x3093, 0x30F3, 0xFF9D}, {0x0000, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000}, {0x0000, 0x0000, 0x0000}} // NN(10)
};




static bool hangul_is_batchim_comport(ucschar ch, ucschar next);
static bool hangul_is_choseong_voiced(ucschar ch);
static bool hangul_is_choseong_p(ucschar ch);
static bool hangul_is_vowel_contracted(ucschar ch);
static ucschar hangul_to_kana_base(ucschar cho, ucschar jung, int type);
static bool hangul_jungseong_split(ucschar ch, ucschar *p_dest1, ucschar *p_dest2);
static ucschar hangul_batchim_to_kana(ucschar cho, ucschar next);
static ucschar hangul_jongseong_to_choseong(ucschar jong);

int hangul_to_kana(ucschar* dest, ucschar prev, ucschar* hangul, ucschar next, int type)
{
    //구현할 부분
    //ucschar key 2개로 kana 문자 맵핑
    // hangul[0] - 초성, hangul[1] - 중성
    // 기본음 + 보조음

    int i=0, j=0; // 초성, 중성 인덱스
    int is_choseong_void=0, is_jungseong_void=0, adjust=0;
    int has_voiced_sound = 0, has_p_sound = 0, has_contracted_sound=0;
    ucschar support = 0;
    int dest_len = 1;

    has_voiced_sound = hangul_is_choseong_voiced(hangul[0]);
    has_p_sound = hangul_is_choseong_p(hangul[0]);
    has_contracted_sound = hangul_is_vowel_contracted(hangul[1]);

    switch(hangul[0]){
        case HANGUL_CHOSEONG_FILLER: i=0; is_choseong_void=1; break;
        case HANJP_CHOSEONG_IEUNG: // ㅇ
            i=0; break;
        case HANJP_CHOSEONG_KHIEUKH: // ㅋ
        case HANJP_CHOSEONG_SSANGKIYEOK: //ㄲ
        case HANJP_CHOSEONG_KIYEOK: // ㄱ // ㅋ -> ㄱ 탁음
            i=1; break;
        case HANJP_CHOSEONG_SIOS: // ㅅ
        case HANJP_CHOSEONG_SSANGSIOS: //ㅆ
        case HANJP_CHOSEONG_CIEUC: // ㅈ // ㅅ -> ㅈ 탁음
            i=2; break;
        case HANJP_CHOSEONG_THIEUTH: // ㅌ
        case HANJP_CHOSEONG_SSANGTIKEUT: //ㄸ
        case HANJP_CHOSEONG_TIKEUT: // ㄷ // ㅌ -> ㄷ 탁음
            i=3; break;
        case HANJP_CHOSEONG_NIEUN: // ㄴ 
            i=4; break; 
        case HANJP_CHOSEONG_HIEUH: // ㅎ
        case HANJP_CHOSEONG_PIEUP: // ㅂ // ㅎ -> ㅂ 탁음
        case HANJP_CHOSEONG_PHIEUPH: // ㅍ // ㅎ -> ㅍ 반탁음
        case HANJP_CHOSEONG_SSANGPIEUP: // ㅃ // 반탁음
            i=5; break; 
        case HANJP_CHOSEONG_MIEUM: // ㅁ
            i=6; break;
        case HANJP_CHOSEONG_RIEUL: // ㄹ
            i=8; break;
        case HANJP_CHOSEONG_PANSIOS: // ㅿ // ㅊ -> ㅿ 탁음
            switch(hangul[1]){
                case HANJP_JUNGSEONG_I:
                case HANJP_JUNGSEONG_U:
                i=3; break;
                default:
                i=2; break;
            }
        case HANJP_CHOSEONG_OLD_IEUNG: // OLD ㅇ
            i = (hangul[1]==HANJP_JUNGSEONG_O)? 12 : 0;
            break;
        default: return -1;
    }

    switch(hangul[1]){
        case HANGUL_JUNGSEONG_FILLER: j=2; is_jungseong_void=1; break;
        case HANJP_JUNGSEONG_A: //ㅏ
        case HANJP_JUNGSEONG_YA: // 야
            j=0; break; 
        case HANJP_JUNGSEONG_I: // ㅣ
            j=1; break; 
        case HANJP_JUNGSEONG_EU: // ㅡ
        case HANJP_JUNGSEONG_U: // ㅜ
        case HANJP_JUNGSEONG_YU: // 유
            j=2; break; 
        case HANJP_JUNGSEONG_AE: // ㅐ
        case HANJP_JUNGSEONG_E: // ㅔ
            j=3; break; 
        case HANJP_JUNGSEONG_O: // ㅗ
        case HANJP_JUNGSEONG_EO:
        case HANJP_JUNGSEONG_YO: // 요
            j=4; break;
        case HANJP_JUNGSEONG_YE:
        case HANJP_JUNGSEONG_YAE:
            j=1; support=kana_table[0][3][type]; break;
        case HANJP_JUNGSEONG_WA: // 와
            if(hangul[1] == HANJP_CHOSEONG_IEUNG){
                j=0;
                support = 0;
            }
            else{
                j=4;
                support = kana_table[0][0][type] - 1;
            }
        default: return -1;
    }

    if(is_choseong_void && is_jungseong_void) {
        dest[0] = 0;
        return 0;
    }

    if(!is_choseong_void && is_jungseong_void && hangul_is_jungseong(prev)){ //받침일 경우
        if(hangul_is_batchim_comport(hangul[0], next)){
            switch(hangul[0]){
                case HANJP_CHOSEONG_SSANGNIEUN:
                case HANJP_CHOSEONG_NIEUN:
                case HANJP_CHOSEONG_MIEUM:
                dest[0] = kana_table[10][0][type];
                return 1;
                case HANJP_CHOSEONG_KIYEOK:
                case HANJP_CHOSEONG_SIOS:
                case HANJP_CHOSEONG_SSANGSIOS:
                case HANJP_CHOSEONG_PIEUP:
                dest[0] = kana_table[2][3][type] - 1;
                return 1;
            }
        }
    }

    if(has_voiced_sound){
        adjust = 1;
    }
    else if(has_p_sound){
        adjust = 2;
    }
    else if(is_choseong_void){
        adjust = -1;
    }
    else{
        adjust = 0;
    }

    if(has_contracted_sound) {
        if(i == 0){
            i = 7;
        }
        else {
            support = kana_table[7][j][type] - 1;
            j = 1;
        }
    }

    switch(hangul[0]){
        case HANJP_CHOSEONG_CHIEUCH:
        case HANJP_CHOSEONG_PANSIOS:
        switch(hangul[1]){
            case HANJP_JUNGSEONG_A:
            case HANJP_JUNGSEONG_E:
            case HANJP_JUNGSEONG_O:
            j = 1; // ㅣ
            support = kana_table[0][j][type] - 1;
            break;
        }
        break;
    }

    dest[0] = kana_table[i][j][type] + adjust;
    
    if(support != 0){
        dest[1] = support;
        dest_len++;
    }
    dest[dest_len] = 0;

    return dest_len;
}

static bool hangul_is_batchim_comport(ucschar ch, ucschar next)
{
    bool res;

    switch(ch){
        case HANJP_CHOSEONG_IEUNG:
        switch(next){
            case HANJP_CHOSEONG_KHIEUKH:
            case HANJP_CHOSEONG_SSANGKIYEOK:
            case HANJP_CHOSEONG_KIYEOK:
            res = true;
            default:
            res = false;
        } break;
        case HANJP_CHOSEONG_KIYEOK:
        switch(next){
            case HANJP_CHOSEONG_KHIEUKH:
            case HANJP_CHOSEONG_SSANGKIYEOK:
            res = true; break;
            default:
            res = false;
        } break;
        case HANJP_CHOSEONG_SIOS:
        switch(next){
            case HANJP_CHOSEONG_SIOS:
            case HANJP_CHOSEONG_SSANGSIOS:
            res = true; break;
            default:
            res = false;
        } break;
        case HANJP_CHOSEONG_NIEUN:
        switch(next){
            case HANJP_CHOSEONG_SIOS:
            case HANJP_CHOSEONG_SSANGSIOS:
            case HANJP_CHOSEONG_THIEUTH:
            case HANJP_CHOSEONG_SSANGTIKEUT:
            case HANJP_CHOSEONG_TIKEUT:
            case HANJP_CHOSEONG_NIEUN:
            case HANJP_CHOSEONG_RIEUL:
            res = true; break;
            default:
            res = false;
        } break;
        case HANJP_CHOSEONG_PIEUP:
        switch(next){
            case HANJP_CHOSEONG_PHIEUPH:
            case HANJP_CHOSEONG_SSANGPIEUP:
            res = true; break;
            default:
            res = false;
        } break;
        case HANJP_CHOSEONG_MIEUM:
        switch(next){
            case HANJP_CHOSEONG_MIEUM:
            case HANJP_CHOSEONG_PIEUP:
            case HANJP_CHOSEONG_PHIEUPH:
            case HANJP_CHOSEONG_SSANGPIEUP:
            res = true; break;
            default:
            res = false;
        } break;
        case HANJP_CHOSEONG_SSANGSIOS: 
        case HANJP_CHOSEONG_SSANGNIEUN:
        res = true; break;
        default: res = false;
    }

    return res;
}

static bool hangul_is_choseong_voiced(ucschar ch)
{
    switch(ch){
        case HANJP_CHOSEONG_KIYEOK:
        case HANJP_CHOSEONG_CIEUC:
        case HANJP_CHOSEONG_TIKEUT:
        case HANJP_CHOSEONG_PANSIOS:
        case HANJP_CHOSEONG_PIEUP:
        return true;
        default:
        return false;
    }
}

static bool hangul_is_choseong_p(ucschar ch)
{
    switch(ch){
        case HANJP_CHOSEONG_PHIEUPH:
        case HANJP_CHOSEONG_SSANGPIEUP:
        return true;
        default:
        return false;
    }
}

static bool hangul_is_vowel_contracted(ucschar ch)
{
    switch(ch){
        case HANJP_JUNGSEONG_YA:
        case HANJP_JUNGSEONG_YU:
        case HANJP_JUNGSEONG_YO:
        return true;
        default:
        return false;
    }
}

static ucschar hangul_to_kana_base(ucschar cho, ucschar jung, int type) // 처리 불가능한 모음은 분리되어서 들어와야함
{
    int i=0, j=0; // 초성, 중성 인덱스
    int is_choseong_void=0, is_jungseong_void=0;
    int has_voiced_sound = 0, has_p_sound = 0, has_contracted_sound;
    ucschar ret;

    switch(cho){
        case HANJP_CHOSEONG_IEUNG: // ㅇ
            i=0; break;
        case HANJP_CHOSEONG_KHIEUKH: // ㅋ
        case HANJP_CHOSEONG_SSANGKIYEOK: //ㄲ
        case HANJP_CHOSEONG_KIYEOK: // ㄱ // ㅋ -> ㄱ 탁음
            i=1; break;
        case HANJP_CHOSEONG_SIOS: // ㅅ
        case HANJP_CHOSEONG_SSANGSIOS: //ㅆ
        case HANJP_CHOSEONG_CIEUC: // ㅈ // ㅅ -> ㅈ 탁음
            i=2; break;
        case HANJP_CHOSEONG_THIEUTH: // ㅌ
        case HANJP_CHOSEONG_SSANGTIKEUT: //ㄸ
        case HANJP_CHOSEONG_TIKEUT: // ㄷ // ㅌ -> ㄷ 탁음
            i=3; break;
        case HANJP_CHOSEONG_NIEUN: // ㄴ 
            i=4; break; 
        case HANJP_CHOSEONG_HIEUH: // ㅎ
        case HANJP_CHOSEONG_PIEUP: // ㅂ // ㅎ -> ㅂ 탁음
        case HANJP_CHOSEONG_PHIEUPH: // ㅍ // ㅎ -> ㅍ 반탁음
        case HANJP_CHOSEONG_SSANGPIEUP: // ㅃ // 반탁음
            i=5; break; 
        case HANJP_CHOSEONG_MIEUM: // ㅁ
            i=6; break;
        case HANJP_CHOSEONG_RIEUL: // ㄹ
            i=8; break;
        case HANJP_CHOSEONG_PANSIOS: // ㅿ // ㅊ -> ㅿ 탁음
            switch(jung){
                case HANJP_JUNGSEONG_I:
                case HANJP_JUNGSEONG_U:
                i=3; break;
                default:
                i=2; break;
            }
        case HANJP_CHOSEONG_OLD_IEUNG: // OLD ㅇ
            i = (jung==HANJP_JUNGSEONG_O)? 9 : 0; //(W or A)
            break;
        default: 
            i=0; is_choseong_void=1;
    }

    switch(jung){
        case HANJP_JUNGSEONG_A: //ㅏ
        case HANJP_JUNGSEONG_YA: // 야
            j=0; break; 
        case HANJP_JUNGSEONG_I: // ㅣ
            j=1; break; 
        case HANJP_JUNGSEONG_EU: // ㅡ
        case HANJP_JUNGSEONG_U: // ㅜ
        case HANJP_JUNGSEONG_YU: // 유
            j=2; break; 
        case HANJP_JUNGSEONG_AE: // ㅐ
        case HANJP_JUNGSEONG_E: // ㅔ
            j=3; break; 
        case HANJP_JUNGSEONG_O: // ㅗ
        case HANJP_JUNGSEONG_EO:
        case HANJP_JUNGSEONG_YO: // 요
            j=4; break;
        case HANJP_JUNGSEONG_YE:
        case HANJP_JUNGSEONG_YAE:
            j=1; break;
        case HANJP_JUNGSEONG_WA: // 와
            j = (cho == HANJP_CHOSEONG_IEUNG) ? 0 : 4;
        default:
            j=2; is_jungseong_void=1;
    }

    if(is_choseong_void && is_jungseong_void){
        return 0;
    }

    if(is_choseong_void){ // small letter
        switch(type)
        {
            case HANJP_OUTPUT_JP_HIRAGANA:
            case HANJP_OUTPUT_JP_KATAKANA:
                ret = kana_table[i][j][type] - 1;
                break;
            case HANJP_OUTPUT_JP_HALF_KATAKANA:
                ret = kana_table[i][j][type] - kana_table[0][0][type] + HALF_KATAKANA_SMALL_A;
                break;
            default:
                return 0;
        }
    }
    else{
        ret = kana_table[i][j][type];
    }
    
    return ret;
}

int _hangul_to_kana(ucschar* dest, ucschar prev, ucschar* hangul, ucschar next, int type){
    int is_choseong_void, is_jungseong_void;
    int return_len = 0;
    int has_voiced_sound, has_p_sound;
    ucschar jungseong1, jungseong2;
    ucschar base = 0, support = 0;
 
    if(hangul_is_jungseong(prev) && !is_choseong_void && is_jungseong_void) //받침 구현
    {
        dest[0] = hangul_batchim_to_kana(hangul[0], next);
        return 1;
    }

    has_voiced_sound = hangul_is_choseong_voiced(hangul[0]);
    has_p_sound = hangul_is_choseong_p(hangul[0]);

    if(hangul_jungseong_split(hangul[1], &jungseong1, &jungseong2))
    {
        base = hangul_to_kana_base(hangul[0], jungseong1, type);
        support = hangul_to_kana_base(HANJP_CHOSEONG_FILLER, jungseong2, type)
    }
    else
    {
        base = hangul_to_kana_base(hangul[0], hangul[1], type);
    }

    if(base) //add
    {
        switch(type) //add base to dest
        {
            case HANJP_OUTPUT_JP_HIRAGANA:
            case HANJP_OUTPUT_JP_KATAKANA:
            if(has_voiced_sound){
                dest[return_len++] = base + 1;
            }
            else if(has_p_sound){
                dest[return_len++] = base + 2;
            }
            else{
                dest[return_len++] = base;
            }
            break;
            case HANJP_OUTPUT_JP_HALF_KATAKANA:
            dest[return_len++] = base;
            if(has_voiced_sound){
                dest[return_len++] = HALF_KATAKANA_VOICED_MARK;
            }
            else if(has_p_sound){
                dest[return_len++] = HALF_KATAKANA_SEMI_VOICED_MARK;
            }
            break;
            default:
            return -1;
        }
        if(support)
            dest[return_len++] = support;
    }


    return return_len;
}

int _hangul_to_kana_full(ucschar* dest, ucschar* hangul, ucschar next, int type){
    int is_choseong_void, is_jungseong_void;
    int return_len = 0;
    int has_voiced_sound, has_p_sound;
    ucschar jungseong1, jungseong2;
    ucschar base = 0, support = 0;
 
    if(hangul_is_jungseong(prev) && !is_choseong_void && is_jungseong_void) //받침 구현
    {
        dest[0] = hangul_batchim_to_kana(hangul[0], next);
        return 1;
    }

    has_voiced_sound = hangul_is_choseong_voiced(hangul[0]);
    has_p_sound = hangul_is_choseong_p(hangul[0]);

    if(hangul_jungseong_split(hangul[1], &jungseong1, &jungseong2))
    {
        base = hangul_to_kana_base(hangul[0], jungseong1, type);
        support = hangul_to_kana_base(HANJP_CHOSEONG_FILLER, jungseong2, type)
    }
    else
    {
        base = hangul_to_kana_base(hangul[0], hangul[1], type);
    }

    if(base) //add
    {
        switch(type) //add base to dest
        {
            case HANJP_OUTPUT_JP_HIRAGANA:
            case HANJP_OUTPUT_JP_KATAKANA:
            if(has_voiced_sound){
                dest[return_len++] = base + 1;
            }
            else if(has_p_sound){
                dest[return_len++] = base + 2;
            }
            else{
                dest[return_len++] = base;
            }
            break;
            case HANJP_OUTPUT_JP_HALF_KATAKANA:
            dest[return_len++] = base;
            if(has_voiced_sound){
                dest[return_len++] = HALF_KATAKANA_VOICED_MARK;
            }
            else if(has_p_sound){
                dest[return_len++] = HALF_KATAKANA_SEMI_VOICED_MARK;
            }
            break;
            default:
            return -1;
        }
        if(support)
            dest[return_len++] = support;
    }

    if(hangul[2]) //받침 구현
        dest[return_len++] = hangul_batchim_to_kana(hangul_jongseong_to_choseong(hangul[2]), next);


    return return_len;
}

static bool hangul_jungseong_split(ucschar ch, ucschar *p_dest1, ucschar *p_dest2)
{
    //to do
    *p_dest1 = 0;
    *p_dest2 = 0;
    return false;
}

static ucschar hangul_batchim_to_kana(ucschar cho, ucschar next)
{
    ucschar ret;

    if(hangul_is_batchim_comport(hangul[0], next)){
            switch(hangul[0]){
                case HANJP_CHOSEONG_SSANGNIEUN:
                case HANJP_CHOSEONG_NIEUN:
                case HANJP_CHOSEONG_MIEUM:
                ret = kana_table[10][0][type];
                break;
                case HANJP_CHOSEONG_KIYEOK:
                case HANJP_CHOSEONG_SIOS:
                case HANJP_CHOSEONG_SSANGSIOS:
                case HANJP_CHOSEONG_PIEUP:
                ret = kana_table[2][3][type] - 1;
                break;
            }
        }
        else{
            switch(hangul[0]){
                case HANJP_CHOSEONG_SSANGNIEUN:
                ret = kana_table[10][0][type];
                break;
                case HANJP_CHOSEONG_NIEUN:
                ret = kana_table[4][2][type];
                break;
                case HANJP_CHOSEONG_MIEUM:
                ret = kana_table[6][2][type];
                break;
                case HANJP_CHOSEONG_KIYEOK:
                ret = kana_table[1][2][type];
                break;
                case HANJP_CHOSEONG_SIOS:
                case HANJP_CHOSEONG_SSANGSIOS:
                ret = kana_table[3][4][type];
                break;
                case HANJP_CHOSEONG_PIEUP:
                ret = kana_table[5][2][type] + 2;
                break;
                default:
                ret = hangul_to_kana_base(hangul[0], HANGUL_JUNGSEONG_FILLER, type);
            }
        }

        return ret;
}