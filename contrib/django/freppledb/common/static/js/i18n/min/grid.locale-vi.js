(function(a){var b={isRTL:!1,defaults:{recordtext:"View {0} - {1} of {2}",emptyrecords:"Kh\u00f4ng c\u00f3 d\u1eef li\u1ec7u",loadtext:"\u0110ang n\u1ea1p d\u1eef li\u1ec7u...",pgtext:"Trang {0} trong t\u1ed5ng s\u1ed1 {1}",pgfirst:"First Page",pglast:"Last Page",pgnext:"Next Page",pgprev:"Previous Page",pgrecs:"Records per Page",showhide:"Toggle Expand Collapse Grid",savetext:"\u0110ang l\u01b0u..."},search:{caption:"T\u00ecm ki\u1ebfm...",Find:"T\u00ecm",Reset:"Kh\u1edfi t\u1ea1o l\u1ea1i",odata:[{oper:"eq",
text:"b\u1eb1ng"},{oper:"ne",text:"kh\u00f4ng b\u1eb1ng"},{oper:"lt",text:"b\u00e9 h\u01a1n"},{oper:"le",text:"b\u00e9 h\u01a1n ho\u1eb7c b\u1eb1ng"},{oper:"gt",text:"l\u1edbn h\u01a1n"},{oper:"ge",text:"l\u1edbn h\u01a1n ho\u1eb7c b\u1eb1ng"},{oper:"bw",text:"b\u1eaft \u0111\u1ea7u v\u1edbi"},{oper:"bn",text:"kh\u00f4ng b\u1eaft \u0111\u1ea7u v\u1edbi"},{oper:"in",text:"trong"},{oper:"ni",text:"kh\u00f4ng n\u1eb1m trong"},{oper:"ew",text:"k\u1ebft th\u00fac v\u1edbi"},{oper:"en",text:"kh\u00f4ng k\u1ebft th\u00fac v\u1edbi"},
{oper:"cn",text:"ch\u1ee9a"},{oper:"nc",text:"kh\u00f4ng ch\u1ee9a"},{oper:"nu",text:"is null"},{oper:"nn",text:"is not null"}],groupOps:[{op:"V\u00c0",text:"t\u1ea5t c\u1ea3"},{op:"HO\u1eb6C",text:"b\u1ea5t k\u1ef3"}],operandTitle:"Click to select search operation.",resetTitle:"Reset Search Value"},edit:{addCaption:"Th\u00eam b\u1ea3n ghi",editCaption:"S\u1eeda b\u1ea3n ghi",bSubmit:"G\u1eedi",bCancel:"H\u1ee7y b\u1ecf",bClose:"\u0110\u00f3ng",saveData:"D\u1eef li\u1ec7u \u0111\u00e3 thay \u0111\u1ed5i! C\u00f3 l\u01b0u thay \u0111\u1ed5i kh\u00f4ng?",
bYes:"C\u00f3",bNo:"Kh\u00f4ng",bExit:"H\u1ee7y b\u1ecf",msg:{required:"Tr\u01b0\u1eddng d\u1eef li\u1ec7u b\u1eaft bu\u1ed9c c\u00f3",number:"H\u00e3y \u0111i\u1ec1n \u0111\u00fang s\u1ed1",minValue:"gi\u00e1 tr\u1ecb ph\u1ea3i l\u1edbn h\u01a1n ho\u1eb7c b\u1eb1ng v\u1edbi ",maxValue:"gi\u00e1 tr\u1ecb ph\u1ea3i b\u00e9 h\u01a1n ho\u1eb7c b\u1eb1ng",email:"kh\u00f4ng ph\u1ea3i l\u00e0 m\u1ed9t email \u0111\u00fang",integer:"H\u00e3y \u0111i\u1ec1n \u0111\u00fang s\u1ed1 nguy\u00ean",date:"H\u00e3y \u0111i\u1ec1n \u0111\u00fang ng\u00e0y th\u00e1ng",
url:"kh\u00f4ng ph\u1ea3i l\u00e0 URL. Kh\u1edfi \u0111\u1ea7u b\u1eaft bu\u1ed9c l\u00e0 ('http://' ho\u1eb7c 'https://')",nodefined:" ch\u01b0a \u0111\u01b0\u1ee3c \u0111\u1ecbnh ngh\u0129a!",novalue:" gi\u00e1 tr\u1ecb tr\u1ea3 v\u1ec1 b\u1eaft bu\u1ed9c ph\u1ea3i c\u00f3!",customarray:"H\u00e0m n\u00ean tr\u1ea3 v\u1ec1 m\u1ed9t m\u1ea3ng!",customfcheck:"Custom function should be present in case of custom checking!"}},view:{caption:"Xem b\u1ea3n ghi",bClose:"\u0110\u00f3ng"},del:{caption:"X\u00f3a",
msg:"X\u00f3a b\u1ea3n ghi \u0111\u00e3 ch\u1ecdn?",bSubmit:"X\u00f3a",bCancel:"H\u1ee7y b\u1ecf"},nav:{edittext:"",edittitle:"S\u1eeda d\u00f2ng \u0111\u00e3 ch\u1ecdn",addtext:"",addtitle:"Th\u00eam m\u1edbi 1 d\u00f2ng",deltext:"",deltitle:"X\u00f3a d\u00f2ng \u0111\u00e3 ch\u1ecdn",searchtext:"",searchtitle:"T\u00ecm b\u1ea3n ghi",refreshtext:"",refreshtitle:"N\u1ea1p l\u1ea1i l\u01b0\u1edbi",alertcap:"C\u1ea3nh b\u00e1o",alerttext:"H\u00e3y ch\u1ecdn m\u1ed9t d\u00f2ng",viewtext:"",viewtitle:"Xem d\u00f2ng \u0111\u00e3 ch\u1ecdn"},
col:{caption:"Ch\u1ecdn c\u1ed9t",bSubmit:"OK",bCancel:"H\u1ee7y b\u1ecf"},errors:{errcap:"L\u1ed7i",nourl:"kh\u00f4ng url \u0111\u01b0\u1ee3c \u0111\u1eb7t",norecords:"Kh\u00f4ng c\u00f3 b\u1ea3n ghi \u0111\u1ec3 x\u1eed l\u00fd",model:"Chi\u1ec1u d\u00e0i c\u1ee7a colNames <> colModel!"},formatter:{integer:{thousandsSeparator:".",defaultValue:"0"},number:{decimalSeparator:",",thousandsSeparator:".",decimalPlaces:2,defaultValue:"0"},currency:{decimalSeparator:",",thousandsSeparator:".",decimalPlaces:2,
prefix:"",suffix:"",defaultValue:"0"},date:{dayNames:"CN;T2;T3;T4;T5;T6;T7;Ch\u1ee7 nh\u1eadt;Th\u1ee9 hai;Th\u1ee9 ba;Th\u1ee9 t\u01b0;Th\u1ee9 n\u0103m;Th\u1ee9 s\u00e1u;Th\u1ee9 b\u1ea3y".split(";"),monthNames:"Th1;Th2;Th3;Th4;Th5;Th6;Th7;Th8;Th9;Th10;Th11;Th12;Th\u00e1ng m\u1ed9t;Th\u00e1ng hai;Th\u00e1ng ba;Th\u00e1ng t\u01b0;Th\u00e1ng n\u0103m;Th\u00e1ng s\u00e1u;Th\u00e1ng b\u1ea3y;Th\u00e1ng t\u00e1m;Th\u00e1ng ch\u00edn;Th\u00e1ng m\u01b0\u1eddi;Th\u00e1ng m\u01b0\u1eddi m\u1ed9t;Th\u00e1ng m\u01b0\u1eddi hai".split(";"),
AmPm:["s\u00e1ng","chi\u1ec1u","S\u00c1NG","CHI\u1ec0U"],S:function(a){return 11>a||13<a?["st","nd","rd","th"][Math.min((a-1)%10,3)]:"th"},srcformat:"Y-m-d",newformat:"n/j/Y",masks:{ShortDate:"n/j/Y",LongDate:"l, F d, Y",FullDateTime:"l, F d, Y g:i:s A",MonthDay:"F d",ShortTime:"g:i A",LongTime:"g:i:s A",YearMonth:"F, Y"}}}};a.jgrid=a.jgrid||{};a.extend(!0,a.jgrid,{defaults:{locale:"vi"},locales:{vi:a.extend({},b,{name:"Ti\u00ea\u0301ng Vi\u1ec7t",nameEnglish:"Vietnamese"}),"vi-VN":a.extend({},b,
{name:"Ti\u00ea\u0301ng Vi\u1ec7t (Vi\u1ec7t Nam)",nameEnglish:"Vietnamese (Vietnam)"})}})})(jQuery);
