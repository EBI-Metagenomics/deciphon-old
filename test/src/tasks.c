#include "cass/cass.h"
#include "dcp/dcp.h"
#include "pparr.h"

void test_tasks(void);

int main(void)
{
    test_tasks();
    return cass_status();
}

#define LOGLIKS(i) SHAPE(24, 2, 2, i)
#define GET_SHAPE(i) LOGLIKS(i)
static imm_float logliks[] = {
    [_(15, 0, 0)] = -90.4046316247, [_(15, 0, 1)] = -97.0227853067, [_(12, 0, 0)] = -92.6975357100,
    [_(12, 0, 1)] = -97.0227853067, [_(13, 0, 0)] = -91.7190648579, [_(13, 0, 1)] = -97.0227853067,
    [_(14, 0, 0)] = -94.4524112790, [_(14, 0, 1)] = -97.0227853067, [_(9, 0, 0)] = -92.8996977877,
    [_(9, 0, 1)] = -97.0227853067,  [_(11, 0, 0)] = -94.0011969160, [_(11, 0, 1)] = -97.0227853067,
    [_(10, 0, 0)] = -92.9036581138, [_(10, 0, 1)] = -97.0227853067, [_(8, 0, 0)] = -93.1729721397,
    [_(8, 0, 1)] = -97.0227853067,  [_(7, 0, 0)] = -93.1375529431,  [_(7, 0, 1)] = -97.0227853067,
    [_(6, 0, 0)] = -91.2324146650,  [_(6, 0, 1)] = -97.0227853067,  [_(5, 0, 0)] = -92.1941132634,
    [_(5, 0, 1)] = -97.0227853067,  [_(4, 0, 0)] = -92.7778877974,  [_(4, 0, 1)] = -97.0227853067,
    [_(3, 0, 0)] = -93.2369686998,  [_(3, 0, 1)] = -97.0227853067,  [_(2, 0, 0)] = -92.7186702764,
    [_(2, 0, 1)] = -97.0227853067,  [_(1, 0, 0)] = -93.1458382777,  [_(1, 0, 1)] = -97.0227853067,
    [_(0, 0, 0)] = -65.4520444198,  [_(0, 0, 1)] = -97.0227853067,  [_(23, 0, 0)] = -93.0179665339,
    [_(23, 0, 1)] = -97.0227853067, [_(22, 0, 0)] = -99.7555094914, [_(22, 0, 1)] = -97.0227853067,
    [_(21, 0, 0)] = -90.5414888321, [_(21, 0, 1)] = -97.0227853067, [_(20, 0, 0)] = -90.7751120505,
    [_(20, 0, 1)] = -97.0227853067, [_(19, 0, 0)] = -92.1835051118, [_(19, 0, 1)] = -97.0227853067,
    [_(16, 0, 0)] = -91.5502243346, [_(16, 0, 1)] = -97.0227853067, [_(18, 0, 0)] = -92.1629236735,
    [_(18, 0, 1)] = -97.0227853067, [_(17, 0, 0)] = -93.3460632620, [_(17, 0, 1)] = -97.0227853067};
#undef GET_SHAPE

#define PATHS(i) SHAPE(24, 2, 2, i)
#define GET_SHAPE(i) PATHS(i)
static char const* paths[] = {
    [_(13, 0, 0)] = "S:0,B:0,M54:3,M55:3,M56:3,M57:3,I57:3,I57:3,I57:3,M58:3,M59:3,M60:3,M61:3,M62:3,M63:3,M64:3,M65:3,"
                    "M66:3,M67:3,M68:3,M69:3,M70:3,E:0,T:0",
    [_(13, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(14, 0, 0)] = "S:0,B:0,M23:3,M24:2,M25:3,M26:3,M27:3,M28:3,M29:3,M30:3,M31:3,M32:3,M33:3,I33:3,M34:3,M35:3,M36:3,"
                    "M37:3,M38:3,M39:3,M40:4,E:0,C:3,T:0",
    [_(14, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(12, 0, 0)] = "S:0,B:0,M213:3,M214:3,I214:3,M215:3,M216:3,M217:3,M218:3,M219:3,M220:3,M221:3,M222:3,M223:3,M224:"
                    "3,M225:3,M226:3,M227:3,M228:3,M229:3,M230:3,M231:3,E:0,T:0",
    [_(12, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(11, 0, 0)] = "S:0,B:0,M68:3,M69:3,M70:3,M71:3,M72:3,D73:0,M74:3,M75:2,M76:3,M77:3,M78:3,M79:3,M80:3,M81:3,M82:3,"
                    "M83:3,M84:3,M85:3,M86:3,M87:4,M88:3,E:0,T:0",
    [_(11, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(9, 0, 0)] = "S:0,B:0,M188:2,M189:3,M190:3,M191:3,M192:3,M193:3,M194:3,M195:3,M196:3,M197:3,M198:3,M199:3,M200:3,"
                   "M201:3,M202:3,I202:3,M203:3,M204:3,M205:3,M206:4,E:0,T:0",
    [_(9, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(10, 0, 0)] = "S:0,B:0,M5:3,M6:3,M7:3,M8:3,M9:3,M10:3,M11:3,M12:3,M13:3,M14:3,M15:3,M16:3,M17:3,M18:3,M19:3,M20:"
                    "3,M21:3,M22:3,M23:3,M24:3,E:0,T:0",
    [_(10, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(8, 0, 0)] = "S:0,B:0,M30:3,M31:3,M32:3,M33:3,M34:3,M35:3,M36:3,M37:3,M38:3,M39:3,M40:3,M41:3,M42:3,M43:3,I43:3,"
                   "M44:3,M45:3,M46:3,M47:3,M48:3,E:0,T:0",
    [_(8, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(7, 0, 0)] = "S:0,B:0,M38:3,I38:2,I38:3,I38:3,I38:3,M39:3,M40:3,M41:3,M42:3,M43:3,M44:3,M45:3,M46:3,M47:3,M48:3,"
                   "M49:3,M50:4,M51:3,M52:3,M53:3,E:0,T:0",
    [_(7, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(6, 0, 0)] = "S:0,B:0,M36:3,M37:3,M38:3,D39:0,D40:0,D41:0,M42:3,M43:3,M44:3,M45:3,M46:3,M47:3,M48:3,M49:3,M50:3,"
                   "M51:3,M52:3,M53:3,M54:3,M55:3,M56:3,M57:3,M58:3,E:0,T:0",
    [_(6, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(5, 0, 0)] = "S:0,B:0,M13:3,M14:3,M15:3,M16:3,M17:3,M18:3,D19:0,M20:3,M21:3,M22:3,M23:3,M24:3,M25:3,M26:3,M27:3,"
                   "M28:3,M29:3,M30:3,M31:3,M32:3,M33:3,E:0,T:0",
    [_(5, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(4, 0, 0)] = "S:0,B:0,M37:3,M38:3,M39:3,M40:3,M41:3,M42:3,M43:3,M44:3,M45:3,M46:3,M47:3,M48:3,M49:3,M50:3,M51:3,"
                   "M52:3,M53:3,M54:3,M55:3,M56:3,E:0,T:0",
    [_(4, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(3, 0, 0)] = "S:0,B:0,M84:3,M85:3,M86:3,M87:3,M88:3,M89:3,M90:3,M91:3,M92:3,M93:3,M94:3,M95:3,M96:3,M97:3,M98:3,"
                   "M99:3,M100:3,M101:3,M102:3,E:0,C:3,T:0",
    [_(3, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(2, 0, 0)] = "S:0,B:0,M67:3,M68:3,M69:3,M70:3,M71:3,M72:3,M73:3,M74:3,M75:3,M76:3,M77:3,M78:3,M79:3,M80:3,M81:3,"
                   "M82:3,M83:3,M84:3,M85:3,M86:3,E:0,T:0",
    [_(2, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(1, 0, 0)] = "S:0,B:0,M13:3,M14:3,M15:3,M16:3,M17:3,M18:3,M19:3,M20:3,M21:3,M22:3,M23:3,M24:3,M25:3,M26:3,M27:3,"
                   "M28:3,M29:3,M30:3,M31:3,M32:3,E:0,T:0",
    [_(1, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(0, 0, 0)] = "S:0,B:0,M1:3,M2:3,M3:3,M4:3,M5:3,M6:3,M7:3,M8:3,M9:3,M10:3,M11:3,M12:3,M13:3,M14:3,M15:3,M16:3,M17:"
                   "3,E:0,J:3,B:0,M20:3,M21:3,E:0,T:0",
    [_(0, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(23, 0, 0)] = "S:0,B:0,M85:3,M86:3,M87:3,M88:3,M89:3,M90:3,M91:3,M92:3,M93:3,M94:3,M95:3,M96:3,M97:3,M98:3,M99:3,"
                    "M100:3,M101:3,M102:3,M103:3,M104:3,E:0,T:0",
    [_(23, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(21, 0, 0)] = "S:0,B:0,M139:3,M140:3,M141:2,M142:3,M143:3,M144:3,M145:3,M146:3,M147:3,M148:3,M149:4,M150:3,M151:"
                    "3,M152:3,M153:3,M154:3,M155:3,M156:3,M157:3,M158:3,E:0,T:0",
    [_(21, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(22, 0, 0)] = "S:0,N:3,B:0,M20:3,M21:3,M22:2,M23:3,M24:3,M25:3,M26:3,M27:3,M28:3,M29:3,M30:3,M31:3,M32:3,M33:2,"
                    "M34:2,M35:3,M36:3,M37:3,M38:3,M39:3,E:0,T:0",
    [_(22, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(20, 0, 0)] = "S:0,B:0,M25:3,M26:3,M27:3,M28:3,M29:3,M30:3,M31:3,M32:3,M33:3,M34:3,M35:3,M36:3,M37:3,M38:3,M39:3,"
                    "M40:3,M41:3,M42:3,M43:3,M44:3,E:0,T:0",
    [_(20, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(19, 0, 0)] = "S:0,B:0,M107:3,M108:3,M109:3,M110:3,M111:3,M112:3,M113:3,M114:3,M115:3,M116:3,M117:3,M118:3,M119:"
                    "3,M120:3,M121:3,M122:3,M123:3,M124:3,M125:3,M126:3,E:0,T:0",
    [_(19, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(18, 0, 0)] = "S:0,B:0,M6:3,M7:3,M8:3,M9:3,M10:3,M11:3,M12:3,M13:3,M14:3,M15:3,M16:3,M17:3,M18:3,M19:3,M20:3,M21:"
                    "3,M22:3,M23:3,M24:3,M25:3,E:0,T:0",
    [_(18, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(17, 0, 0)] = "S:0,B:0,M66:3,M67:3,M68:3,M69:3,M70:3,M71:3,M72:3,M73:3,M74:3,M75:3,M76:3,M77:3,M78:3,M79:3,M80:3,"
                    "M81:3,M82:3,M83:3,M84:3,M85:3,E:0,T:0",
    [_(17, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(16, 0, 0)] = "S:0,B:0,M288:3,M289:3,M290:3,M291:3,M292:3,M293:3,M294:3,M295:3,M296:3,M297:3,M298:3,M299:3,M300:"
                    "3,M301:3,M302:3,M303:3,M304:3,M305:3,M306:3,M307:3,E:0,T:0",
    [_(16, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
    [_(15, 0, 0)] = "S:0,B:0,M152:3,M153:3,M154:3,M155:3,M156:2,M157:3,M158:3,M159:3,M160:3,M161:3,M162:3,M163:3,M164:"
                    "3,M165:3,M166:3,M167:3,M168:3,M169:3,M170:3,M171:4,E:0,T:0",
    [_(15, 0, 1)] = "R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3,R:3",
};
#undef GET_SHAPE

#define CODONS(i) SHAPE(24, 2, 2, i)
#define GET_SHAPE(i) CODONS(i)
static char const* codons[] = {
    [_(19, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(19, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(18, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(18, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(16, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(16, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(17, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(17, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(15, 0, 0)] = "ATGCGCCGCAACGCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCCG",
    [_(15, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(13, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(13, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(12, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(12, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(14, 0, 0)] = "ATGCCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGCGCG",
    [_(14, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(9, 0, 0)] = "ATTGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGC",
    [_(9, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(11, 0, 0)] = "ATGCGCCGCAACCGCATGTATTGCGACCATTATTACCACCACCATTACCACCCTGGCGCG",
    [_(11, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(10, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(10, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(8, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(8, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(7, 0, 0)] = "ATGCAGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCTGGGCGCG",
    [_(7, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(6, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(6, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(5, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(5, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(4, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(4, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(3, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(3, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(2, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(2, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(1, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(1, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(0, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(0, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(23, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(23, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(21, 0, 0)] = "ATGCGCCTGCAACCGCATGATTGCGACCATTATACCACCACCATTACCACCCTGGGCGCG",
    [_(21, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(22, 0, 0)] = "ATGCGCCGCAATCCGCATGATTGCGACCATTATTACCACCACCACTTTACCACCCTGGGCGCG",
    [_(22, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(20, 0, 0)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
    [_(20, 0, 1)] = "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG",
};
#undef GET_SHAPE

void test_tasks(void)
{
    char const* filepath = "/Users/horta/tmp/pfam24.dcp";

    struct dcp_server* server = dcp_server_create(filepath);
    cass_not_null(server);

    cass_equal(dcp_server_start(server), 0);

    struct dcp_task_cfg cfgs[4] = {{true, true, false, false, true},
                                   {true, true, true, false, true},
                                   {true, true, false, true, true},
                                   {true, true, true, true, true}};

    for (unsigned k = 0; k < 4; ++k) {
        struct dcp_task* task = dcp_task_create(cfgs[k]);
        /* >Leader_Thr-sample1 */
        /* MRRNRMIATIITTTITTLGAG */
        dcp_task_add_seq(task, "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCG");
        dcp_task_add_seq(task,
                         "ATGCGCCGCAACCGCATGATTGCGACCATTATTACCACCACCATTACCACCCTGGGCGCGCAGATGCGCCGCAACCGCATGATTGCGACCA"
                         "TTATTACCACCACCATTACCACCCTGGGCGCG");
        dcp_server_add_task(server, task);

        printf("task_end: %d\n", dcp_task_end(task));
        while (!dcp_task_end(task)) {
            printf("Ponto 1\n");
            struct dcp_results* results = dcp_task_read(task);
            if (!results) {
                dcp_sleep(50);
                continue;
            }
            printf("Ponto 2\n");

            for (uint16_t i = 0; i < dcp_results_size(results); ++i) {
                struct dcp_result const* r = dcp_results_get(results, i);
                uint32_t                 seqid = dcp_result_seqid(r);
                uint32_t                 profid = dcp_result_profid(r);

                for (unsigned j = 0; j < 2; ++j) {

                    enum dcp_model m = dcp_models[j];
                    /* #define GET_SHAPE(i) LOGLIKS(i) */
                    /*                 cass_close(dcp_result_loglik(r, m), logliks[_(profid, seqid, j)]); */
                    /* #undef GET_SHAPE */

                    /* #define GET_SHAPE(i) PATHS(i) */
                    /*                 char const* path = dcp_string_data(dcp_result_path(r, m)); */
                    /*                 cass_equal(strcmp(path, paths[_(profid, seqid, j)]), 0); */
                    /* #undef GET_SHAPE */

                    /* #define GET_SHAPE(i) CODONS(i) */
                    /*                 char const* codon = dcp_string_data(dcp_result_codons(r, m)); */
                    /*                 cass_equal(strcmp(codon, codons[_(profid, seqid, j)]), 0); */
                    /* #undef GET_SHAPE */
                    struct dcp_task_cfg cfg = dcp_task_cfg(task);
                    printf("[_(%d, %d, %d, %d, %d)] = %.10f,\n", profid, seqid, j, cfg.hmmer3_compat, cfg.multiple_hits,
                           dcp_result_loglik(r, m));
                    printf("[_(%d, %d, %d, %d, %d)] = \"%s\",\n", profid, seqid, j, cfg.hmmer3_compat,
                           cfg.multiple_hits, dcp_string_data(dcp_result_path(r, m)));
                    printf("[_(%d, %d, %d, %d, %d)] = \"%s\",\n", profid, seqid, j, cfg.hmmer3_compat,
                           cfg.multiple_hits, dcp_string_data(dcp_result_codons(r, m)));
                }
            }
            dcp_server_free_results(server, results);
        }
        dcp_server_free_task(server, task);
        printf("--------------------------------------------\n");
    }

    dcp_server_stop(server);

    cass_equal(dcp_server_destroy(server), 0);
}
