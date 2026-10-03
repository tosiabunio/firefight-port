  IDEAL
  P386
  MODEL FLAT,C

public font_hi

CODESEG

macro OnePixel pos
  mov [edi+pos],al
endm

macro TwoPixels pos
  mov [edi+pos],ax
endm

macro ThreePixels pos
  mov [edi+pos],ax
  mov [edi+pos+2],al
endm

macro FourPixels pos
  mov [edi+pos],eax
endm

macro NewLine
  add edi,edx
endm

macro Skip2Lines
  lea edi,[edi+edx*2]
endm


character_000:   ; znak #00
character_001:   ; znak #01
character_002:   ; znak #02
character_003:   ; znak #03
character_004:   ; znak #04
character_005:   ; znak #05
character_006:   ; znak #06
character_007:   ; znak #07
character_008:   ; znak #08
character_009:   ; znak #09
character_010:   ; znak #10
character_011:   ; znak #11
character_012:   ; znak #12
character_013:   ; znak #13
character_014:   ; znak #14
character_015:   ; znak #15
character_016:   ; znak #16
character_017:   ; znak #17
character_018:   ; znak #18
character_019:   ; znak #19
character_020:   ; znak #20
character_021:   ; znak #21
character_022:   ; znak #22
character_023:   ; znak #23
character_024:   ; znak #24
character_025:   ; znak #25
character_026:   ; znak #26
character_027:   ; znak #27
character_028:   ; znak #28
character_029:   ; znak #29
character_030:   ; znak #30
character_031:   ; znak #31
character_128:   ; znak 'Ç'
character_129:   ; znak 'ü'
character_130:   ; znak 'é'
character_131:   ; znak 'â'
character_132:   ; znak 'ä'
character_133:   ; znak 'ů'
character_135:   ; znak 'ç'
character_137:   ; znak 'ë'
character_138:   ; znak 'Ő'
character_139:   ; znak 'ő'
character_140:   ; znak 'î'
character_142:   ; znak 'Ä'
character_144:   ; znak 'É'
character_145:   ; znak 'Ĺ'
character_146:   ; znak 'ĺ'
character_147:   ; znak 'ô'
character_148:   ; znak 'ö'
character_149:   ; znak 'Ľ'
character_150:   ; znak 'ľ'
character_153:   ; znak 'Ö'
character_154:   ; znak 'Ü'
character_155:   ; znak 'Ť'
character_156:   ; znak 'ť'
character_158:   ; znak '×'
character_159:   ; znak 'č'
character_160:   ; znak 'á'
character_161:   ; znak 'í'
character_163:   ; znak 'ú'
character_166:   ; znak 'Ž'
character_167:   ; znak 'ž'
character_170:   ; znak '¬'
character_172:   ; znak 'Č'
character_173:   ; znak 'ş'
character_174:   ; znak '«'
character_175:   ; znak '»'
character_176:   ; znak '░'
character_177:   ; znak '▒'
character_178:   ; znak '▓'
character_179:   ; znak '│'
character_180:   ; znak '┤'
character_181:   ; znak 'Á'
character_182:   ; znak 'Â'
character_183:   ; znak 'Ě'
character_184:   ; znak 'Ş'
character_185:   ; znak '╣'
character_186:   ; znak '║'
character_187:   ; znak '╗'
character_188:   ; znak '╝'
character_191:   ; znak '┐'
character_192:   ; znak '└'
character_193:   ; znak '┴'
character_194:   ; znak '┬'
character_195:   ; znak '├'
character_196:   ; znak '─'
character_197:   ; znak '┼'
character_198:   ; znak 'Ă'
character_199:   ; znak 'ă'
character_200:   ; znak '╚'
character_201:   ; znak '╔'
character_202:   ; znak '╩'
character_203:   ; znak '╦'
character_204:   ; znak '╠'
character_205:   ; znak '═'
character_206:   ; znak '╬'
character_207:   ; znak '¤'
character_208:   ; znak 'đ'
character_209:   ; znak 'Đ'
character_210:   ; znak 'Ď'
character_211:   ; znak 'Ë'
character_212:   ; znak 'ď'
character_213:   ; znak 'Ň'
character_214:   ; znak 'Í'
character_215:   ; znak 'Î'
character_216:   ; znak 'ě'
character_217:   ; znak '┘'
character_218:   ; znak '┌'
character_219:   ; znak '█'
character_220:   ; znak '▄'
character_221:   ; znak 'Ţ'
character_222:   ; znak 'Ů'
character_223:   ; znak '▀'
character_225:   ; znak 'ß'
character_226:   ; znak 'Ô'
character_229:   ; znak 'ň'
character_230:   ; znak 'Š'
character_231:   ; znak 'š'
character_232:   ; znak 'Ŕ'
character_233:   ; znak 'Ú'
character_234:   ; znak 'ŕ'
character_235:   ; znak 'Ű'
character_236:   ; znak 'ý'
character_237:   ; znak 'Ý'
character_238:   ; znak 'ţ'
character_239:   ; znak '´'
character_240:   ; znak '­'
character_241:   ; znak '˝'
character_242:   ; znak '˛'
character_243:   ; znak 'ˇ'
character_244:   ; znak '˘'
character_245:   ; znak '§'
character_246:   ; znak '÷'
character_247:   ; znak '¸'
character_248:   ; znak '°'
character_249:   ; znak '¨'
character_250:   ; znak '˙'
character_251:   ; znak 'ű'
character_252:   ; znak 'Ř'
character_253:   ; znak 'ř'
character_254:   ; znak '■'
character_255:   ; znak ' '
character_032:   ; znak ' '
  ret

character_033:   ; znak '!'
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  Skip2Lines
  TwoPixels 3
  ret

character_034:   ; znak '"'
  TwoPixels 2
  TwoPixels 5
  NewLine
  TwoPixels 1
  TwoPixels 4
  ret

character_035:   ; znak '#'
  TwoPixels 1
  TwoPixels 4
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 1
  TwoPixels 4
  ret

character_036:   ; znak '$'
  TwoPixels 3
  NewLine
  FourPixels 2
  OnePixel 6
  NewLine
  TwoPixels 1
  NewLine
  FourPixels 2
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 3
  ret

character_037:   ; znak '%'
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  OnePixel 0
  TwoPixels 5
  ret

character_038:   ; znak '&'
  TwoPixels 2
  NewLine
  TwoPixels 1
  OnePixel 4
  NewLine
  TwoPixels 2
  NewLine
  ThreePixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 3
  NewLine
  TwoPixels 0
  TwoPixels 3
  NewLine
  ThreePixels 1
  OnePixel 5
  ret

character_039:   ; znak '''
  TwoPixels 3
  NewLine
  TwoPixels 2
  ret

character_040:   ; znak '('
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 4
  ret

character_041:   ; znak ')'
  TwoPixels 1
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  ret

character_042:   ; znak '*'
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  ThreePixels 2
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  ThreePixels 2
  NewLine
  TwoPixels 1
  TwoPixels 4
  ret

character_043:   ; znak '+'
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  ret

character_044:   ; znak ','
  Skip2Lines
  Skip2Lines
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  ret

character_045:   ; znak '-'
  Skip2Lines
  NewLine
  FourPixels 1
  TwoPixels 5
  ret

character_046:   ; znak '.'
  Skip2Lines
  Skip2Lines
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  ret

character_047:   ; znak '/'
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 0
  NewLine
  OnePixel 0
  ret

character_048:   ; znak '0'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  OnePixel 3
  TwoPixels 5
  NewLine
  ThreePixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_049:   ; znak '1'
  TwoPixels 3
  NewLine
  ThreePixels 2
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  FourPixels 2
  ret

character_050:   ; znak '2'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  FourPixels 0
  ThreePixels 4
  ret

character_051:   ; znak '3'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_052:   ; znak '4'
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 5
  ret

character_053:   ; znak '5'
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 1
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_054:   ; znak '6'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_055:   ; znak '7'
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 0
  ret

character_056:   ; znak '8'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_057:   ; znak '9'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_058:   ; znak ':'
  Skip2Lines
  TwoPixels 3
  NewLine
  TwoPixels 3
  Skip2Lines
  TwoPixels 3
  NewLine
  TwoPixels 3
  ret

character_059:   ; znak ';'
  Skip2Lines
  TwoPixels 3
  NewLine
  TwoPixels 3
  Skip2Lines
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  ret

character_060:   ; znak '<'
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 4
  ret

character_061:   ; znak '='
  Skip2Lines
  FourPixels 1
  TwoPixels 5
  Skip2Lines
  FourPixels 1
  TwoPixels 5
  ret

character_062:   ; znak '>'
  TwoPixels 2
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  ret

character_063:   ; znak '?'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 3
  Skip2Lines
  TwoPixels 3
  ret

character_064:   ; znak '@'
  NewLine
  FourPixels 1
  NewLine
  TwoPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  ThreePixels 3
  NewLine
  TwoPixels 0
  ThreePixels 3
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_065:   ; znak 'A'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_066:   ; znak 'B'
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_067:   ; znak 'C'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_068:   ; znak 'D'
  FourPixels 0
  OnePixel 4
  NewLine
  TwoPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_069:   ; znak 'E'
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  ThreePixels 4
  ret

character_070:   ; znak 'F'
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  OnePixel 4
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  ret

character_071:   ; znak 'G'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_072:   ; znak 'H'
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_073:   ; znak 'I'
  FourPixels 2
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  FourPixels 2
  ret

character_074:   ; znak 'J'
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_075:   ; znak 'K'
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 3
  NewLine
  FourPixels 0
  NewLine
  TwoPixels 0
  TwoPixels 3
  NewLine
  TwoPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_076:   ; znak 'L'
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  ThreePixels 4
  ret

character_077:   ; znak 'M'
  TwoPixels 0
  TwoPixels 5
  NewLine
  ThreePixels 0
  ThreePixels 4
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  OnePixel 3
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_078:   ; znak 'N'
  TwoPixels 0
  TwoPixels 5
  NewLine
  ThreePixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  FourPixels 3
  NewLine
  TwoPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_079:   ; znak 'O'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_080:   ; znak 'P'
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  ret

character_081:   ; znak 'Q'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 4
  NewLine
  ThreePixels 1
  TwoPixels 5
  ret

character_082:   ; znak 'R'
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 4
  NewLine
  FourPixels 0
  OnePixel 4
  NewLine
  TwoPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_083:   ; znak 'S'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_084:   ; znak 'T'
  FourPixels 0
  FourPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  ret

character_085:   ; znak 'U'
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_086:   ; znak 'V'
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  ThreePixels 2
  ret

character_087:   ; znak 'W'
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  OnePixel 3
  TwoPixels 5
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  ThreePixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_088:   ; znak 'X'
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  ThreePixels 2
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_089:   ; znak 'Y'
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_090:   ; znak 'Z'
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  ThreePixels 4
  ret

character_091:   ; znak '['
  ThreePixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  ThreePixels 2
  ret

character_092:   ; znak '\'
  TwoPixels 0
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 5
  NewLine
  OnePixel 6
  ret

character_093:   ; znak ']'
  ThreePixels 1
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  ThreePixels 1
  ret

character_094:   ; znak '^'
  OnePixel 3
  NewLine
  ThreePixels 2
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_095:   ; znak '_'
  Skip2Lines
  Skip2Lines
  Skip2Lines
  NewLine
  FourPixels 0
  FourPixels 4
  ret

character_096:   ; znak '`'
  FourPixels 2
  NewLine
  OnePixel 1
  OnePixel 6
  NewLine
  OnePixel 0
  TwoPixels 3
  OnePixel 7
  NewLine
  OnePixel 0
  OnePixel 2
  OnePixel 7
  NewLine
  OnePixel 0
  OnePixel 2
  OnePixel 7
  NewLine
  OnePixel 0
  TwoPixels 3
  OnePixel 7
  NewLine
  OnePixel 1
  OnePixel 6
  NewLine
  FourPixels 2
  ret

character_097:   ; znak 'a'
  NewLine
  FourPixels 2
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  ret

character_098:   ; znak 'b'
  TwoPixels 0
  NewLine
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_099:   ; znak 'c'
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_100:   ; znak 'd'
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  ret

character_101:   ; znak 'e'
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 1
  TwoPixels 5
  ret

character_102:   ; znak 'f'
  FourPixels 2
  OnePixel 6
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 1
  NewLine
  FourPixels 0
  OnePixel 4
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 1
  ret

character_103:   ; znak 'g'
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_104:   ; znak 'h'
  TwoPixels 0
  NewLine
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_105:   ; znak 'i'
  TwoPixels 3
  Skip2Lines
  ThreePixels 2
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 3
  NewLine
  FourPixels 2
  ret

character_106:   ; znak 'j'
  TwoPixels 5
  Skip2Lines
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_107:   ; znak 'k'
  TwoPixels 0
  NewLine
  TwoPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 3
  NewLine
  FourPixels 0
  NewLine
  TwoPixels 0
  TwoPixels 3
  NewLine
  TwoPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_108:   ; znak 'l'
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_109:   ; znak 'm'
  NewLine
  ThreePixels 0
  TwoPixels 4
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  OnePixel 3
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_110:   ; znak 'n'
  NewLine
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_111:   ; znak 'o'
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_112:   ; znak 'p'
  NewLine
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  ret

character_113:   ; znak 'q'
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  ret

character_114:   ; znak 'r'
  NewLine
  TwoPixels 0
  ThreePixels 3
  NewLine
  FourPixels 0
  NewLine
  ThreePixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  ret

character_115:   ; znak 's'
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_116:   ; znak 't'
  TwoPixels 1
  NewLine
  FourPixels 0
  OnePixel 4
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 1
  NewLine
  FourPixels 2
  ret

character_117:   ; znak 'u'
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_118:   ; znak 'v'
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  ThreePixels 2
  NewLine
  OnePixel 3
  ret

character_119:   ; znak 'w'
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  OnePixel 3
  TwoPixels 5
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  ThreePixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_120:   ; znak 'x'
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  ThreePixels 2
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_121:   ; znak 'y'
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_122:   ; znak 'z'
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  FourPixels 0
  ThreePixels 4
  ret

character_123:   ; znak '{'
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 3
  ret

character_124:   ; znak '|'
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  ret

character_125:   ; znak '}'
  TwoPixels 1
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  ret

character_126:   ; znak '~'
  FourPixels 1
  TwoPixels 6
  NewLine
  TwoPixels 0
  FourPixels 3
  ret

character_127:   ; znak #127
  Skip2Lines
  Skip2Lines
  ThreePixels 2
  NewLine
  TwoPixels 1
  TwoPixels 4
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_134:   ; znak 'ć'
  TwoPixels 3
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_136:   ; znak 'ł'
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  OnePixel 3
  NewLine
  ThreePixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_141:   ; znak 'Ź'
  OnePixel 1
  TwoPixels 3
  OnePixel 6
  NewLine
  TwoPixels 2
  OnePixel 5
  NewLine
  OnePixel 4
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  ThreePixels 4
  ret

character_143:   ; znak 'Ć'
  TwoPixels 1
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 3
  OnePixel 6
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_151:   ; znak 'Ś'
  TwoPixels 1
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 3
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_152:   ; znak 'ś'
  TwoPixels 3
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 5
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 4
  ret

character_157:   ; znak 'Ł'
  TwoPixels 0
  NewLine
  TwoPixels 0
  TwoPixels 3
  NewLine
  FourPixels 0
  NewLine
  ThreePixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  ThreePixels 4
  ret

character_162:   ; znak 'ó'
  TwoPixels 3
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_164:   ; znak 'Ą'
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 4
  ret

character_165:   ; znak 'ą'
  NewLine
  FourPixels 2
  NewLine
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 4
  ret

character_168:   ; znak 'Ę'
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  NewLine
  TwoPixels 0
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 4
  ret

character_169:   ; znak 'ę'
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 4
  ret

character_171:   ; znak 'ź'
  TwoPixels 3
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  FourPixels 0
  ThreePixels 4
  ret

character_189:   ; znak 'Ż'
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  TwoPixels 3
  NewLine
  FourPixels 1
  OnePixel 5
  NewLine
  TwoPixels 1
  NewLine
  TwoPixels 0
  NewLine
  FourPixels 0
  ThreePixels 4
  ret

character_190:   ; znak 'ż'
  NewLine
  FourPixels 1
  TwoPixels 5
  NewLine
  TwoPixels 4
  NewLine
  FourPixels 2
  NewLine
  TwoPixels 2
  NewLine
  TwoPixels 1
  NewLine
  FourPixels 0
  ThreePixels 4
  ret

character_224:   ; znak 'Ó'
  TwoPixels 1
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 3
  OnePixel 6
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  FourPixels 1
  OnePixel 5
  ret

character_227:   ; znak 'Ń'
  TwoPixels 0
  FourPixels 3
  NewLine
  ThreePixels 0
  TwoPixels 5
  NewLine
  FourPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  FourPixels 3
  NewLine
  TwoPixels 0
  ThreePixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

character_228:   ; znak 'ń'
  TwoPixels 3
  NewLine
  FourPixels 0
  TwoPixels 4
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  NewLine
  TwoPixels 0
  TwoPixels 5
  ret

DATASEG

font_hi:
  dd character_000   ; znak #00
  dd character_001   ; znak #01
  dd character_002   ; znak #02
  dd character_003   ; znak #03
  dd character_004   ; znak #04
  dd character_005   ; znak #05
  dd character_006   ; znak #06
  dd character_007   ; znak #07
  dd character_008   ; znak #08
  dd character_009   ; znak #09
  dd character_010   ; znak #10
  dd character_011   ; znak #11
  dd character_012   ; znak #12
  dd character_013   ; znak #13
  dd character_014   ; znak #14
  dd character_015   ; znak #15
  dd character_016   ; znak #16
  dd character_017   ; znak #17
  dd character_018   ; znak #18
  dd character_019   ; znak #19
  dd character_020   ; znak #20
  dd character_021   ; znak #21
  dd character_022   ; znak #22
  dd character_023   ; znak #23
  dd character_024   ; znak #24
  dd character_025   ; znak #25
  dd character_026   ; znak #26
  dd character_027   ; znak #27
  dd character_028   ; znak #28
  dd character_029   ; znak #29
  dd character_030   ; znak #30
  dd character_031   ; znak #31
  dd character_032   ; znak ' '
  dd character_033   ; znak '!'
  dd character_034   ; znak '"'
  dd character_035   ; znak '#'
  dd character_036   ; znak '$'
  dd character_037   ; znak '%'
  dd character_038   ; znak '&'
  dd character_039   ; znak '''
  dd character_040   ; znak '('
  dd character_041   ; znak ')'
  dd character_042   ; znak '*'
  dd character_043   ; znak '+'
  dd character_044   ; znak ','
  dd character_045   ; znak '-'
  dd character_046   ; znak '.'
  dd character_047   ; znak '/'
  dd character_048   ; znak '0'
  dd character_049   ; znak '1'
  dd character_050   ; znak '2'
  dd character_051   ; znak '3'
  dd character_052   ; znak '4'
  dd character_053   ; znak '5'
  dd character_054   ; znak '6'
  dd character_055   ; znak '7'
  dd character_056   ; znak '8'
  dd character_057   ; znak '9'
  dd character_058   ; znak ':'
  dd character_059   ; znak ';'
  dd character_060   ; znak '<'
  dd character_061   ; znak '='
  dd character_062   ; znak '>'
  dd character_063   ; znak '?'
  dd character_064   ; znak '@'
  dd character_065   ; znak 'A'
  dd character_066   ; znak 'B'
  dd character_067   ; znak 'C'
  dd character_068   ; znak 'D'
  dd character_069   ; znak 'E'
  dd character_070   ; znak 'F'
  dd character_071   ; znak 'G'
  dd character_072   ; znak 'H'
  dd character_073   ; znak 'I'
  dd character_074   ; znak 'J'
  dd character_075   ; znak 'K'
  dd character_076   ; znak 'L'
  dd character_077   ; znak 'M'
  dd character_078   ; znak 'N'
  dd character_079   ; znak 'O'
  dd character_080   ; znak 'P'
  dd character_081   ; znak 'Q'
  dd character_082   ; znak 'R'
  dd character_083   ; znak 'S'
  dd character_084   ; znak 'T'
  dd character_085   ; znak 'U'
  dd character_086   ; znak 'V'
  dd character_087   ; znak 'W'
  dd character_088   ; znak 'X'
  dd character_089   ; znak 'Y'
  dd character_090   ; znak 'Z'
  dd character_091   ; znak '['
  dd character_092   ; znak '\'
  dd character_093   ; znak ']'
  dd character_094   ; znak '^'
  dd character_095   ; znak '_'
  dd character_096   ; znak '`'
  dd character_097   ; znak 'a'
  dd character_098   ; znak 'b'
  dd character_099   ; znak 'c'
  dd character_100   ; znak 'd'
  dd character_101   ; znak 'e'
  dd character_102   ; znak 'f'
  dd character_103   ; znak 'g'
  dd character_104   ; znak 'h'
  dd character_105   ; znak 'i'
  dd character_106   ; znak 'j'
  dd character_107   ; znak 'k'
  dd character_108   ; znak 'l'
  dd character_109   ; znak 'm'
  dd character_110   ; znak 'n'
  dd character_111   ; znak 'o'
  dd character_112   ; znak 'p'
  dd character_113   ; znak 'q'
  dd character_114   ; znak 'r'
  dd character_115   ; znak 's'
  dd character_116   ; znak 't'
  dd character_117   ; znak 'u'
  dd character_118   ; znak 'v'
  dd character_119   ; znak 'w'
  dd character_120   ; znak 'x'
  dd character_121   ; znak 'y'
  dd character_122   ; znak 'z'
  dd character_123   ; znak '{'
  dd character_124   ; znak '|'
  dd character_125   ; znak '}'
  dd character_126   ; znak '~'
  dd character_127   ; znak #127
  dd character_128   ; znak 'Ç'
  dd character_129   ; znak 'ü'
  dd character_130   ; znak 'é'
  dd character_131   ; znak 'â'
  dd character_132   ; znak 'ä'
  dd character_133   ; znak 'ů'
  dd character_134   ; znak 'ć'
  dd character_135   ; znak 'ç'
  dd character_136   ; znak 'ł'
  dd character_137   ; znak 'ë'
  dd character_138   ; znak 'Ő'
  dd character_139   ; znak 'ő'
  dd character_140   ; znak 'î'
  dd character_141   ; znak 'Ź'
  dd character_142   ; znak 'Ä'
  dd character_143   ; znak 'Ć'
  dd character_144   ; znak 'É'
  dd character_145   ; znak 'Ĺ'
  dd character_146   ; znak 'ĺ'
  dd character_147   ; znak 'ô'
  dd character_148   ; znak 'ö'
  dd character_149   ; znak 'Ľ'
  dd character_150   ; znak 'ľ'
  dd character_151   ; znak 'Ś'
  dd character_152   ; znak 'ś'
  dd character_153   ; znak 'Ö'
  dd character_154   ; znak 'Ü'
  dd character_155   ; znak 'Ť'
  dd character_156   ; znak 'ť'
  dd character_157   ; znak 'Ł'
  dd character_158   ; znak '×'
  dd character_159   ; znak 'č'
  dd character_160   ; znak 'á'
  dd character_161   ; znak 'í'
  dd character_162   ; znak 'ó'
  dd character_163   ; znak 'ú'
  dd character_164   ; znak 'Ą'
  dd character_165   ; znak 'ą'
  dd character_166   ; znak 'Ž'
  dd character_167   ; znak 'ž'
  dd character_168   ; znak 'Ę'
  dd character_169   ; znak 'ę'
  dd character_170   ; znak '¬'
  dd character_171   ; znak 'ź'
  dd character_172   ; znak 'Č'
  dd character_173   ; znak 'ş'
  dd character_174   ; znak '«'
  dd character_175   ; znak '»'
  dd character_176   ; znak '░'
  dd character_177   ; znak '▒'
  dd character_178   ; znak '▓'
  dd character_179   ; znak '│'
  dd character_180   ; znak '┤'
  dd character_181   ; znak 'Á'
  dd character_182   ; znak 'Â'
  dd character_183   ; znak 'Ě'
  dd character_184   ; znak 'Ş'
  dd character_185   ; znak '╣'
  dd character_186   ; znak '║'
  dd character_187   ; znak '╗'
  dd character_188   ; znak '╝'
  dd character_189   ; znak 'Ż'
  dd character_190   ; znak 'ż'
  dd character_191   ; znak '┐'
  dd character_192   ; znak '└'
  dd character_193   ; znak '┴'
  dd character_194   ; znak '┬'
  dd character_195   ; znak '├'
  dd character_196   ; znak '─'
  dd character_197   ; znak '┼'
  dd character_198   ; znak 'Ă'
  dd character_199   ; znak 'ă'
  dd character_200   ; znak '╚'
  dd character_201   ; znak '╔'
  dd character_202   ; znak '╩'
  dd character_203   ; znak '╦'
  dd character_204   ; znak '╠'
  dd character_205   ; znak '═'
  dd character_206   ; znak '╬'
  dd character_207   ; znak '¤'
  dd character_208   ; znak 'đ'
  dd character_209   ; znak 'Đ'
  dd character_210   ; znak 'Ď'
  dd character_211   ; znak 'Ë'
  dd character_212   ; znak 'ď'
  dd character_213   ; znak 'Ň'
  dd character_214   ; znak 'Í'
  dd character_215   ; znak 'Î'
  dd character_216   ; znak 'ě'
  dd character_217   ; znak '┘'
  dd character_218   ; znak '┌'
  dd character_219   ; znak '█'
  dd character_220   ; znak '▄'
  dd character_221   ; znak 'Ţ'
  dd character_222   ; znak 'Ů'
  dd character_223   ; znak '▀'
  dd character_224   ; znak 'Ó'
  dd character_225   ; znak 'ß'
  dd character_226   ; znak 'Ô'
  dd character_227   ; znak 'Ń'
  dd character_228   ; znak 'ń'
  dd character_229   ; znak 'ň'
  dd character_230   ; znak 'Š'
  dd character_231   ; znak 'š'
  dd character_232   ; znak 'Ŕ'
  dd character_233   ; znak 'Ú'
  dd character_234   ; znak 'ŕ'
  dd character_235   ; znak 'Ű'
  dd character_236   ; znak 'ý'
  dd character_237   ; znak 'Ý'
  dd character_238   ; znak 'ţ'
  dd character_239   ; znak '´'
  dd character_240   ; znak '­'
  dd character_241   ; znak '˝'
  dd character_242   ; znak '˛'
  dd character_243   ; znak 'ˇ'
  dd character_244   ; znak '˘'
  dd character_245   ; znak '§'
  dd character_246   ; znak '÷'
  dd character_247   ; znak '¸'
  dd character_248   ; znak '°'
  dd character_249   ; znak '¨'
  dd character_250   ; znak '˙'
  dd character_251   ; znak 'ű'
  dd character_252   ; znak 'Ř'
  dd character_253   ; znak 'ř'
  dd character_254   ; znak '■'
  dd character_255   ; znak ' '
  END
