Þ            ­         Ð
     Ñ
     ×
     ã
     ÷
                 !   (     J     Y     b     j     }            Þ      {  	     	     
   ¢     ­     ´  0   ¹  %   ê               '     :     W     ^     s     z               §     ¶     ½  p   Ò     C     K     a     h  "   y       +   «     ×  $   à                         #     /     5  9   =     w            0   ¬     Ý     ó     ÿ       (        <     J     R  
   ^     i     |            	   ¥     ¯     ·     ¾     Ç     Ð     Õ     Ú     á     í     ö               !     3     E     R     a     f  
   k     v                     ¥     ®     º     È     Ð     ×     Ü     è     ú  
             #     2     ;     D     M  
   S     ^     n     u     |       
             ¤     ±     ¾     Ã     Ï     Õ     Û  ß  á     Á     È  "   Õ  -   ø     &     <     L  )   \                     §     º  	   Á     Ë  >  Û  @  %     ['     n'     '     '     '  E   '  <   ß'     (     #(  !   3(  0   U(     (     (     ²(     ¹(     Æ(     Ü(     ø(     )     )     .)     Í)     Ô)     ó)     ú)  ,   *     C*  H   b*     «*  #   ¾*     â*     é*     ù*      +     +     +     +  S   %+  !   y+     +     ¨+  <   »+     ø+     ,     ,     1,  6   D,     {,     ,     ,     ®,     »,     Ú,     ê,     ú,     -      -     0-     =-     J-     Z-     a-     e-     l-  	   s-     }-     -     -     ¡-     º-     Ð-     Ý-     ê-     ñ-  	   ø-     .     .     %.     5.     <.     C.     \.     u.     |.     .     .     .  	   §.     ±.  !   Ä.     æ.     ü.     /     /     /  	   -/     7/     G/     W/     ^/  	   b/  	   l/     v/  	   /  	   /     /     ¡/     ®/     µ/     ¹/             `              2   V   .      7   k   )   n       N   	          F       o   l                  g       ~   6   '       ?   Q      w   9   #   I                  y         q   =   C   S             x                  R   <          v   |      %   i       p      !   M   J       c   t   A          s       O      +   3       (   }       4   [      {   m   P       u   \   0      e   b                    *   _          E   K             &       j   H          W           ;   8   U           >   Z   1   $      ]   :   B       "   ^   ,   Y      a   5          d             /       
       T   -   z   f   r   h           D   G   X       L   @        Admin Autorefresh Available datasets: Back up the database Bucket size Cancel Canceled Capacity: respect capacity limits Change history Comments Confirm Constrained demand Constraints Copy Create a plan Create a sample model in the database.<br/>
The parameters control the size and complexity.<br/>
Number of end items: <input id="create0" name="clusters" type="text" maxlength="5" size="5" value="100" onchange="calcUtil()"/><br/>
<b>Demand:</b><br/>
&nbsp;&nbsp;Monthly forecast per end item: <input id="create1" name="fcst" type="text" maxlength="4" size="4" value="50" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Demands per end item: <input id="create2" name="demands" type="text" maxlength="4" size="4" value="30" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Average delivery lead time: <input id="create3" name="deliver_lt" type="text" maxlength="4" size="4" value="30" onchange="calcUtil()"/> days<br/>
<b>Raw Materials:</b><br/>
&nbsp;&nbsp;Depth of bill-of-material: <input id="create4" name="levels" type="text" maxlength="2" size="2" value="5" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Total number of components: <input id="create5" name="components" type="text" maxlength="5" size="5" value="200" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Number of components per end item: <input id="create6" name="components_per" type="text" maxlength="5" size="5" value="4" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Average procurement lead time: <input id="create7" name="procure_lt" type="text" maxlength="4" size="4" value="40" onchange="calcUtil()"/> days<br/>
<b>Capacity:</b><br/>
&nbsp;&nbsp;Number of resources: <input id="create8" name="rsrc_number" type="text" maxlength="3" size="3" value="60" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Size of each resource: <input id="create9" name="rsrc_size" type="text" maxlength="3" size="3" value="5" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Expected average resource utilization: <span id="util">76.7</span>&#37;<br/>
 Create time buckets for reporting.<br/>
Start date: <input class="date" name="start" type="text" maxlength="5" size="12"/><br/>
End date: <input class="date" name="end" type="text" maxlength="5" size="12"/><br/>
Week starts on: <select name="weekstart">
<option value="0">Sunday</option>
<option value="1" selected="selected">Monday</option>
<option value="2">Tuesday</option>
<option value="3">Wednesday</option>
<option value="4">Thursday</option>
<option value="5">Friday</option>
<option value="6">Saturday</option>
</select>
 Customize Data file Deliveries Detail Done Download all input data in a single spreadsheet. Dump the database contents to a file. Edit Edit availability Empty the database Error retrieving report data Export Export a spreadsheet Failed Forecast method Gantt chart Generate buckets Generate model Import Import a spreadsheet Import input data from a spreadsheet.<br/>The spreadsheet must match the structure exported with the task above. Inquiry Keep active in memory Launch Launch new tasks Lead time: do not plan in the past Load a dataset Load a dataset from a file in the database. Log file Material: respect procurement limits Move in Move out Pegging Plan Plan detail Quote Release Release fence: do not plan within the release time window Release selected scenarios Reset Scenario management Sorry, You don't have any execute permissions... Stop the web service. Supply Path Time buckets Update Update description of selected scenarios View log file Waiting Web service Where Used Why short or late? Zoom in Zoom out after plan current date available backlog buffer comments consumed date days demand description end date end inventory forecast forecast adjustment forecast baseline forecast consumed forecast net forecast total free from identifier into selected scenarios item last refresh load location locked ends locked starts minimum months name open orders orders adjustment overload parameters planned net forecast planned orders problems produced quantity setup start date start inventory status supply to total demand total ends total orders total starts total supply type unavailable units value weeks Project-Id-Version: 3.0.beta
Report-Msgid-Bugs-To: 
POT-Creation-Date: 2015-10-12 10:56+0200
PO-Revision-Date: 2015-11-25 09:26+0100
Last-Translator: Johan De Taeye <jdetaeye@frepple.com>
Language-Team: Japanese <kde-i18n-doc@kde.org>
Language: ja_JP
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
Plural-Forms: nplurals=1; plural=0;
X-Generator: Poedit 1.8.6
X-Poedit-Basepath: /home/frepple/workspace/frepple/contrib/django/freppledb
 ç®¡ç èªåæ´æ° å©ç¨å¯è½ãªãã¼ã¿ã»ãã: ãã¼ã¿ãã¼ã¹ãããã¯ã¢ãããã ãã±ãããµã¤ãº ã­ã£ã³ã»ã« ã­ã£ã³ã»ã« è½å: è½åä¸ã®éåº¦ãèæ®ãã å¤æ´å±¥æ­´ ã³ã¡ã³ã ç¢ºèª æ¡ä»¶ã¤ãè¦æ± æ¡ä»¶ ã³ãã¼ è¨ç»ãä½æ ã¢ãã«ãä½æ<br/>
ã·ã³ãã«ãªã¢ãã«ããã¼ã¿ãã¼ã¹ã«ä½æãã¾ã<br/>
ãã©ã¡ã¼ã¿ã«ãããµã¤ãºã¨è¤éæ§ãã³ã³ãã­ã¼ã«ãã¾ã<br/>
çµäºã¢ã¤ãã ã®æ°: <input id="create0" name="clusters" type="text" maxlength="5" size="5" value="100" onchange="calcUtil()"/><br/>
<b>éè¦:</b><br/>
&nbsp;&nbsp;æçµè£½åãã¨ã®æéäºæ¸¬: <input id="create1" name="fcst" type="text" maxlength="4" size="4" value="50" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;æçµè£½åãã¨ã®æ³¨æ: <input id="create2" name="demands" type="text" maxlength="4" size="4" value="30" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;åºè·ã¾ã§ã®å¹³åãªã¼ãã¿ã¤ã : <input id="create3" name="deliver_lt" type="text" maxlength="4" size="4" value="30" onchange="calcUtil()"/> days<br/>
<b>åææ:</b><br/>
&nbsp;&nbsp;é¨åæ§æè¡¨ã®æ·±ã: <input id="create4" name="levels" type="text" maxlength="2" size="2" value="5" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;é¨åã®ç·æ°: <input id="create5" name="components" type="text" maxlength="5" size="5" value="200" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;æçµè£½åãã¨ã®é¨åã®æ°: <input id="create6" name="components_per" type="text" maxlength="5" size="5" value="4" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;å¹³åèª¿éãªã¼ãã¿ã¤ã : <input id="create7" name="procure_lt" type="text" maxlength="4" size="4" value="40" onchange="calcUtil()"/> days<br/>
<b>è½å:</b><br/>
&nbsp;&nbsp;ãªã½ã¼ã¹ã®æ°: <input id="create8" name="rsrc_number" type="text" maxlength="3" size="3" value="60" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;ãªã½ã¼ã¹ã®ãµã¤ãº: <input id="create9" name="rsrc_size" type="text" maxlength="3" size="3" value="5" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;æ³å®ãããå¹³åãªã½ã¼ã¹å©ç¨ç: <span id="util">76.7</span>&#37;<br/>
 ã¬ãã¼ãããæéãã±ãããä½æãã¾ãã<br/>
éå§æ¥æ: <input class="date" name="start" type="text" maxlength="5" size="12"/><br/>
çµäºæ¥æ: <input class="date" name="end" type="text" maxlength="5" size="12"/><br/>
é±ã®éå§ææ¥: <select name="weekstart">
<option value="0">æ¥ææ¥</option>
<option value="1" selected="selected">æææ¥</option>
<option value="2">ç«ææ¥</option>
<option value="3">æ°´ææ¥</option>
<option value="4">æ¨ææ¥</option>
<option value="5">éææ¥</option>
<option value="6">åææ¥</option>
</select>
 ã«ã¹ã¿ãã¤ãº ãã¼ã¿ãã¡ã¤ã« åºè· è©³ç´° å®äº ãã¹ã¦ã®å¥åãã¼ã¿ãåä¸ã®è¡¨ã«ãã¦ãã¦ã³ã­ã¼ã ãã¼ã¿ãã¼ã¹ã®åå®¹ããã¡ã¤ã«ã«ãã³ããã ç·¨é è½åãç·¨é ãã¼ã¿ãã¼ã¹ãç©ºã«ãã ã¬ãã¼ããã¼ã¿åå¾ã«å¤±æãã¾ãã ã¨ã¯ã¹ãã¼ã è¡¨ãã¨ã¯ã¹ãã¼ã å¤±æ äºæ¸¬æ¹å¼ ã¬ã³ããã£ã¼ã ã«ã¬ã³ãã¼ãã±ãã æ±ç¨ã¢ãã« ã¤ã³ãã¼ã è¡¨ãã¤ã³ãã¼ã è¡¨ãããã¼ã¿ãã¤ã³ãã¼ããã¾ãã<br/>è¡¨ã¯ã¨ã¯ã¹ãã¼ããããä¸è¨ã®ã¿ã¹ã¯ã¨æ§é ãä¸è´ãããªããã°ãªãã¾ããã ç§ä¼ ã¡ã¢ãªåã§ã¢ã¯ãã£ã èµ·å æ°ããã¿ã¹ã¯ãèµ·å ãªã¼ãã¿ã¤ã : éå»ã¯è¨ç»ããªã ãã¼ã¿ã»ãããã­ã¼ã ãã¼ã¿ãã¼ã¹ä¸­ã®ãã¡ã¤ã«ãããã¼ã¿ã»ãããã­ã¼ã ã­ã°ãã¡ã¤ã« è³æº: èª¿ééåº¦ãèæ®ãã ç§»å¥ ã­ã°ã¢ã¦ã åºå® è¨ç» è¨ç»ã®è©³ç´° å¼ç¨ è§£é¤ ãã§ã³ã¹ãè§£æ¾: è§£æ¾ãããæéã¦ã¤ã³ãã¦ã®ä¸­ã§è¨ç»ããªã é¸æããã·ããªãªãè§£é¤ ãªã»ãã ã·ããªãªç®¡ç ç³ãè¨³ããã¾ãããå®è¡æ¨©éãããã¾ãã... Webãµã¼ãã¹ãåæ­¢ ä¾çµ¦çµè·¯ æéãã±ãã ã¢ãããã¼ã é¸æããã·ããªãªã®è¨è¿°ãã¢ãããã¼ã ã­ã°ãã¡ã¤ã«ãè¦ã å¾æ© Webãµã¼ãã¹ ä½¿ç¨å ´æ ä¸è¶³ã¾ãã¯éå»¶ã®çç± ãºã¼ã ã¤ã³ ã­ã°ã¢ã¦ã ç¾å¨æ¥æããå¾ã®è¨ç» å¯è½ ããã¯ã­ã° ãããã¡ ã³ã¡ã³ã æ¶è²»ããã æ¥æ æ¥ éè¦ è¨è¿° çµäºæ¥ æ£å¸ã®çµäº äºæ¸¬ ä¿®æ­£äºæ¸¬ ãã¼ã¹ã©ã¤ã³äºæ¸¬ æ¶è²»ãããäºæ¸¬ åè¨äºæ¸¬ åè¨äºæ¸¬ è§£æ¾ ãã è­å¥å­ é¸æããã·ããªãªã¸ åå ååã®æ´æ° è² è· å°å ã­ãã¯ãããçµäº ã­ãã¯ãããéå§ æå° æ åå æ³¨æãéã æ³¨æã®ä¿®æ­£ éè² è· ãã©ã¡ã¼ã¿ã¼ è¨ç»æ¸ã¿ã®ãã¼ã¿ã«äºæ¸¬ è¨ç»ãããæ³¨æ åé¡ çç£ããã æ°é ã»ããã¢ãã éå§æ¥ æ£å¸ã®éå§ ã¹ãã¼ã¿ã¹ ä¾çµ¦ ã¸ ç·éè¦ å¨çµäº æ³¨æã®åè¨ å¨éå§ ç·ä¾çµ¦ ç¨®é¡ åå¾ä¸å¯ åä½ å¤ é± 