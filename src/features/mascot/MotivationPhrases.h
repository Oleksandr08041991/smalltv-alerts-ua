// 140 Ukrainian flight/motivation/business quotes, Flash-backed to preserve
// ESP8266 heap for GitHub OTA TLS. All text and the index table are PROGMEM.
// Sources/attribution in docs/mascot-sources.md; adaptions are marked
// «переказ» and original lines credited to fictional character «Авіадруг».
// NEVER dereference Phrase::text/author directly; use copy() to SRAM first.
#pragma once
#include <Arduino.h>
#include <pgmspace.h>
#include <string.h>
namespace motivation {
struct Phrase {const char* text; const char* author; uint8_t business;};
static const char a0[] PROGMEM = "Леся Українка";
static const char a1[] PROGMEM = "Іван Франко";
static const char a2[] PROGMEM = "Франклін • переказ";
static const char a3[] PROGMEM = "Авіадруг";
static const char a4[] PROGMEM = "Зіг Зіглар • переказ";
static const char a5[] PROGMEM = "Р. Кольєр • переказ";
static const char a6[] PROGMEM = "Дж. Дьюї • переказ";
static const char a7[] PROGMEM = "С. Джобс • переказ";
static const char a8[] PROGMEM = "М. Тетчер • переказ";
static const char a9[] PROGMEM = "А. Вінтур • переказ";
static const char a10[] PROGMEM = "Н. Гілл • переказ";
static const char a11[] PROGMEM = "Р. Кійосакі • переказ";
static const char a12[] PROGMEM = "Джим Рон • переказ";
static const char a13[] PROGMEM = "Т. Едісон • переказ";
static const char a14[] PROGMEM = "Стів Джобс • переказ";
static const char a15[] PROGMEM = "Рей Крок • переказ";
static const char a16[] PROGMEM = "М. Обама • переказ";
static const char a17[] PROGMEM = "Дж. Максвелл • переказ";
static const char a18[] PROGMEM = "Гете • переказ";
static const char a19[] PROGMEM = "Колумб • переказ";
static const char a20[] PROGMEM = "Т. Рузвельт • переказ";
static const char a21[] PROGMEM = "П. Друкер • переказ";
static const char a22[] PROGMEM = "С. Жадан • переказ";
static const char a23[] PROGMEM = "А. Ейнштейн • переказ";
static const char a24[] PROGMEM = "С. Кові • переказ";
static const char a25[] PROGMEM = "Конфуцій • переказ";
static const char a26[] PROGMEM = "В. Дісней • переказ";
static const char a27[] PROGMEM = "Сунь Цзи • переказ";
static const char a28[] PROGMEM = "Н. Мандела • переказ";
static const char a29[] PROGMEM = "Р. Бах • переказ";
static const char a30[] PROGMEM = "Ф. Бекон • переказ";
static const char a31[] PROGMEM = "Г. Форд • переказ";
static const char a32[] PROGMEM = "Стів Кові • переказ";
static const char a33[] PROGMEM = "Опра Вінфрі • переказ";
static const char a34[] PROGMEM = "Сократ • переказ";
static const char t0[] PROGMEM = "Без надії таки сподіватись";
static const char t1[] PROGMEM = "Я хочу крізь сльози сміятись";
static const char t2[] PROGMEM = "Буду сіять барвисті квітки";
static const char t3[] PROGMEM = "Лупайте сю скалу!";
static const char t4[] PROGMEM = "Нехай ні жар, ні холод не спинить вас";
static const char t5[] PROGMEM = "Час не повернути, тож бережи його";
static const char t6[] PROGMEM = "Старанність зближує мету й результат";
static const char t7[] PROGMEM = "Малі кроки долають великі перешкоди";
static const char t8[] PROGMEM = "Пильність і розсудливість бережуть справу";
static const char t9[] PROGMEM = "Роби добро і не відкладай";
static const char t10[] PROGMEM = "Думай наперед, щоб не наздоганяти";
static const char t11[] PROGMEM = "Робота швидше за порожні обіцянки";
static const char t12[] PROGMEM = "Знання множаться, коли вчишся";
static const char t13[] PROGMEM = "Дія важливіша за добрі наміри";
static const char t14[] PROGMEM = "Ощадливість дає свободу вибору";
static const char t15[] PROGMEM = "Лінь робить шлях довшим";
static const char t16[] PROGMEM = "Дбай про час, а не тільки про гроші";
static const char t17[] PROGMEM = "Навчайся у кожній справі";
static const char t18[] PROGMEM = "Пильнуй дрібниці, що творять ціле";
static const char t19[] PROGMEM = "Не марнуй часу на зайві суперечки";
static const char t20[] PROGMEM = "Працюй завчасно, а не поспіхом";
static const char t21[] PROGMEM = "Порядок допомагає мислити чітко";
static const char t22[] PROGMEM = "Обіцяв? Доведи справу до кінця";
static const char t23[] PROGMEM = "Добра репутація потребує щоденної праці";
static const char t24[] PROGMEM = "Дій сьогодні, не чекай завтра";
static const char t25[] PROGMEM = "Кожна звичка формує результат";
static const char t26[] PROGMEM = "Малі витрати теж мають значення";
static const char t27[] PROGMEM = "Практика перетворює знання на вміння";
static const char t28[] PROGMEM = "Не втрачай уваги до деталей";
static const char t29[] PROGMEM = "Працюй мудро і з гарним настроєм";
static const char t30[] PROGMEM = "Тримай курс і віру в себе";
static const char t31[] PROGMEM = "Рухайся вперед";
static const char t32[] PROGMEM = "Сьогодні твоя висота";
static const char t33[] PROGMEM = "Крила ростуть у польоті";
static const char t34[] PROGMEM = "Новий день — новий рейс";
static const char t35[] PROGMEM = "Усмішка допоможе злетіти";
static const char t36[] PROGMEM = "Твоя ідея варта спроби";
static const char t37[] PROGMEM = "Не втрачай свого горизонту";
static const char t38[] PROGMEM = "Відкрий крила можливостям";
static const char t39[] PROGMEM = "Один крок — теж прогрес";
static const char t40[] PROGMEM = "Тримай висоту настрою";
static const char t41[] PROGMEM = "Злітай із радістю";
static const char t42[] PROGMEM = "Не бійся першого кроку";
static const char t43[] PROGMEM = "Курс на нові вершини";
static const char t44[] PROGMEM = "Відпочинок — частина польоту";
static const char t45[] PROGMEM = "Кожен світанок дарує шанс";
static const char t46[] PROGMEM = "Пошукай попутний вітер";
static const char t47[] PROGMEM = "Нехай день буде світлим";
static const char t48[] PROGMEM = "Ти здатен на більше";
static const char t49[] PROGMEM = "Починай із простого";
static const char t50[] PROGMEM = "Керуй своїм днем";
static const char t51[] PROGMEM = "Май сміливість мріяти";
static const char t52[] PROGMEM = "Вір у свій екіпаж";
static const char t53[] PROGMEM = "Поступ — це перемога";
static const char t54[] PROGMEM = "Зберігай спокій і курс";
static const char t55[] PROGMEM = "Дякуй собі за старанність";
static const char t56[] PROGMEM = "Працюй з усмішкою";
static const char t57[] PROGMEM = "Час пізнати нове";
static const char t58[] PROGMEM = "Зроби добру справу";
static const char t59[] PROGMEM = "Не забувай радіти";
static const char t60[] PROGMEM = "Сильна команда — сильний політ";
static const char t61[] PROGMEM = "Мета має бути зрозумілою";
static const char t62[] PROGMEM = "Якість починається з уваги";
static const char t63[] PROGMEM = "Дій чітко, перевір результат";
static const char t64[] PROGMEM = "Довіра тримається на діях";
static const char t65[] PROGMEM = "Розвивай те, що працює";
static const char t66[] PROGMEM = "Краще рішення — перевірене";
static const char t67[] PROGMEM = "Навчання підсилює команду";
static const char t68[] PROGMEM = "Усміхнись колезі";
static const char t69[] PROGMEM = "Ідеї люблять підтримку";
static const char t70[] PROGMEM = "Гарний план знімає тривогу";
static const char t71[] PROGMEM = "Вимірюй важливе";
static const char t72[] PROGMEM = "Будь прикладом для команди";
static const char t73[] PROGMEM = "Починай із потреби клієнта";
static const char t74[] PROGMEM = "Питай, перш ніж вирішувати";
static const char t75[] PROGMEM = "Досконалість росте з практики";
static const char t76[] PROGMEM = "Слухай уважніше";
static const char t77[] PROGMEM = "Малий крок змінює систему";
static const char t78[] PROGMEM = "Цінуй час свого екіпажу";
static const char t79[] PROGMEM = "Обирай сміливі рішення";
static const char t80[] PROGMEM = "Фокус допомагає команді";
static const char t81[] PROGMEM = "Роби складне зрозумілим";
static const char t82[] PROGMEM = "Спільна мета об’єднує";
static const char t83[] PROGMEM = "Тримай слово";
static const char t84[] PROGMEM = "Гарне питання веде далі";
static const char t85[] PROGMEM = "Пам’ятай про результат";
static const char t86[] PROGMEM = "Підтримуй ініціативу";
static const char t87[] PROGMEM = "Не зупиняй розвиток";
static const char t88[] PROGMEM = "Змінюй маршрут, не мету";
static const char t89[] PROGMEM = "Мисли ширше";
static const char t90[] PROGMEM = "Сьогодні підкоримо нову висоту";
static const char t91[] PROGMEM = "Пілотуй свою ідею";
static const char t92[] PROGMEM = "Знайди привід для радості";
static const char t93[] PROGMEM = "Сміливість любить дію";
static const char t94[] PROGMEM = "Думай на кілька кроків";
static const char t95[] PROGMEM = "Будуй, випробовуй, покращуй";
static const char t96[] PROGMEM = "Ділись здобутим досвідом";
static const char t97[] PROGMEM = "Краще поступово, але надійно";
static const char t98[] PROGMEM = "Май план на завтра";
static const char t99[] PROGMEM = "Політ триває";
static const char t100[] PROGMEM = "Сміливо почни, навіть без готовності";
static const char t101[] PROGMEM = "Великі результати ростуть зі щоденних справ";
static const char t102[] PROGMEM = "Навчайся, доки дієш";
static const char t103[] PROGMEM = "Ідеї втілюються завдяки дії";
static const char t104[] PROGMEM = "Починай до появи натхнення";
static const char t105[] PROGMEM = "Крок за кроком піднімайся вище";
static const char t106[] PROGMEM = "Дій, навіть коли не все ідеально";
static const char t107[] PROGMEM = "Мета починається з чіткого рішення";
static const char t108[] PROGMEM = "Міняй звички, щоб міняти результати";
static const char t109[] PROGMEM = "Керуй днем, поки він не керує тобою";
static const char t110[] PROGMEM = "Вчорашній досвід — основа нового кроку";
static const char t111[] PROGMEM = "Вибирай справу, яка надихає";
static const char t112[] PROGMEM = "Сила росте там, де є наполегливість";
static const char t113[] PROGMEM = "Помилки теж навчають перемагати";
static const char t114[] PROGMEM = "Навіть маленький успіх вартий уваги";
static const char t115[] PROGMEM = "Мрія потребує сміливого старту";
static const char t116[] PROGMEM = "Відвага допомагає знайти новий берег";
static const char t117[] PROGMEM = "Цінуй свій поступ, не лише вершину";
static const char t118[] PROGMEM = "Упевненість починає великі справи";
static const char t119[] PROGMEM = "Створи майбутнє своїми діями";
static const char t120[] PROGMEM = "Вчись і дій — це сильна команда";
static const char t121[] PROGMEM = "Працьовитість перетворює мрії на план";
static const char t122[] PROGMEM = "Не бійся спробувати новий шлях";
static const char t123[] PROGMEM = "Сміливість допомагає почати";
static const char t124[] PROGMEM = "Гарні можливості часто приховані у змінах";
static const char t125[] PROGMEM = "Надійний поступ сильніший за ривок";
static const char t126[] PROGMEM = "Залишайся допитливим і відкритим";
static const char t127[] PROGMEM = "Тримай план і будь готовий змінювати його";
static const char t128[] PROGMEM = "Продовжуй, навіть якщо дорога складна";
static const char t129[] PROGMEM = "Твій досвід зростає з кожною спробою";
static const char t130[] PROGMEM = "Збирай маленькі перемоги щодня";
static const char t131[] PROGMEM = "Знання варте дії";
static const char t132[] PROGMEM = "Вір у власні сили та рухайся";
static const char t133[] PROGMEM = "Довіра до команди народжується у діях";
static const char t134[] PROGMEM = "Працюй із задоволенням від процесу";
static const char t135[] PROGMEM = "Будь сміливим у рішеннях";
static const char t136[] PROGMEM = "Розвиток починається з питання";
static const char t137[] PROGMEM = "Мисли ширше за межі звички";
static const char t138[] PROGMEM = "Твоя наполегливість робить різницю";
static const char t139[] PROGMEM = "Не відкладай важливе на потім";
static const Phrase bank[] PROGMEM = {
  {t0,a0,0},
  {t1,a0,0},
  {t2,a0,0},
  {t3,a1,0},
  {t4,a1,0},
  {t5,a2,1},
  {t6,a2,1},
  {t7,a2,1},
  {t8,a2,1},
  {t9,a2,0},
  {t10,a2,1},
  {t11,a2,1},
  {t12,a2,1},
  {t13,a2,1},
  {t14,a2,1},
  {t15,a2,1},
  {t16,a2,1},
  {t17,a2,0},
  {t18,a2,1},
  {t19,a2,1},
  {t20,a2,1},
  {t21,a2,1},
  {t22,a2,1},
  {t23,a2,1},
  {t24,a2,1},
  {t25,a2,0},
  {t26,a2,1},
  {t27,a2,1},
  {t28,a2,1},
  {t29,a2,0},
  {t30,a3,0},
  {t31,a3,0},
  {t32,a3,0},
  {t33,a3,0},
  {t34,a3,0},
  {t35,a3,0},
  {t36,a3,0},
  {t37,a3,0},
  {t38,a3,0},
  {t39,a3,0},
  {t40,a3,0},
  {t41,a3,0},
  {t42,a3,0},
  {t43,a3,0},
  {t44,a3,0},
  {t45,a3,0},
  {t46,a3,0},
  {t47,a3,0},
  {t48,a3,0},
  {t49,a3,0},
  {t50,a3,0},
  {t51,a3,0},
  {t52,a3,0},
  {t53,a3,0},
  {t54,a3,0},
  {t55,a3,0},
  {t56,a3,0},
  {t57,a3,0},
  {t58,a3,0},
  {t59,a3,0},
  {t60,a3,1},
  {t61,a3,1},
  {t62,a3,1},
  {t63,a3,1},
  {t64,a3,1},
  {t65,a3,1},
  {t66,a3,1},
  {t67,a3,1},
  {t68,a3,1},
  {t69,a3,1},
  {t70,a3,1},
  {t71,a3,1},
  {t72,a3,1},
  {t73,a3,1},
  {t74,a3,1},
  {t75,a3,1},
  {t76,a3,1},
  {t77,a3,1},
  {t78,a3,1},
  {t79,a3,1},
  {t80,a3,1},
  {t81,a3,1},
  {t82,a3,1},
  {t83,a3,1},
  {t84,a3,1},
  {t85,a3,1},
  {t86,a3,1},
  {t87,a3,1},
  {t88,a3,1},
  {t89,a3,1},
  {t90,a3,0},
  {t91,a3,0},
  {t92,a3,0},
  {t93,a3,0},
  {t94,a3,0},
  {t95,a3,0},
  {t96,a3,0},
  {t97,a3,0},
  {t98,a3,0},
  {t99,a3,0},
  {t100,a4,1},
  {t101,a5,1},
  {t102,a6,1},
  {t103,a7,1},
  {t104,a4,0},
  {t105,a8,1},
  {t106,a9,1},
  {t107,a10,1},
  {t108,a11,1},
  {t109,a12,1},
  {t110,a13,1},
  {t111,a14,0},
  {t112,a15,1},
  {t113,a16,0},
  {t114,a17,1},
  {t115,a18,0},
  {t116,a19,0},
  {t117,a8,0},
  {t118,a20,0},
  {t119,a21,1},
  {t120,a6,1},
  {t121,a22,1},
  {t122,a23,0},
  {t123,a4,1},
  {t124,a24,1},
  {t125,a25,0},
  {t126,a26,0},
  {t127,a27,1},
  {t128,a28,0},
  {t129,a29,0},
  {t130,a17,1},
  {t131,a30,1},
  {t132,a31,0},
  {t133,a32,1},
  {t134,a33,0},
  {t135,a21,1},
  {t136,a34,0},
  {t137,a11,1},
  {t138,a5,1},
  {t139,a12,1},
};
static constexpr uint8_t count=sizeof(bank)/sizeof(bank[0]);
inline Phrase get(uint8_t idx){
  Phrase p{};
  memcpy_P(&p,&bank[idx%count],sizeof(p));
  return p;
}
inline void copy(uint8_t idx,char* txt,size_t tCap,char* author,size_t aCap){
  Phrase p=get(idx);
  if(txt&&tCap){strncpy_P(txt,p.text,tCap-1);txt[tCap-1]=0;}
  if(author&&aCap){strncpy_P(author,p.author,aCap-1);author[aCap-1]=0;}
}
} // namespace motivation
