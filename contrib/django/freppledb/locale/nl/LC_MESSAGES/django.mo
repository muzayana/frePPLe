��    �      �  �   �	      �     �     �               4     @     G  !   P     r     �     �     �     �     �     �     �     �     �  �  �     �    �  	   �  	          
   )     4     ;     I     W  0   \  %   �     �     �     �  &   �       +   !     M     T     i     �     �     �     �     �     �     �     �               +  I   @     �  p   �               2     9  "   J     m  +   |     �  $   �  .   �                         #     /     5     :  9   B     |     �     �     �  0   �     �                    :     G  (   N     w     �     �  
   �     �     �     �     �     �  	   �     �     �     �                    "     '     .     :     C     Q     Z     n     �     �     �     �     �  
   �     �     �     �     �     �     �                    $     )  
   2     =     I     [  
   d     o     �     �     �     �     �  
   �     �     �     �     �     �  
   �     �                    $     0     6     <  �  B     1!     ?!     T!     j!     �!     �!     �!  -   �!  	   �!     �!  
   �!     �!     "     "     ""     ."     6"     U"  �  f"     ^)    r)     �+     �+      �+  
   �+     �+     �+  
   �+     �+  /   �+  ,   &,  	   S,     ],     w,  /   �,  %   �,     �,  	   �,     -     -     ;-  ,   W-     �-     �-     �-     �-     �-     �-  	   �-     �-     .  X   .     x.  �   �.     /     /     :/     B/  /   W/     �/  /   �/     �/  )   �/  6   �/     50     =0     C0     K0     P0     \0     b0  	   g0  <   q0      �0     �0     �0     �0  D   �0     >1  
   R1     ]1     k1     �1     �1  /   �1     �1     �1  
   �1     �1     2     !2     )2     12     :2     N2     Z2     f2  
   m2     x2     �2     �2     �2     �2     �2  	   �2     �2     �2     �2     �2     �2     3     !3     13     63  
   :3     E3     c3     k3     z3     3     �3     �3     �3     �3     �3     �3     �3     �3     �3     �3  
   �3     	4     #4  	   34     =4     W4     ^4  
   d4     o4     }4     �4     �4     �4     �4     �4     �4     �4     �4     �4     �4     �4     �4     #              {   K   x   q   h              Z          P       d           �      R   w   '              |       /   :   ?   �   %       `   4   9   �   f   ,   �   U       C      c       =   "       }           O   0       �      !   �      s              n              �   L   v   �   r      6   �           �   N   ]   �   [   �   1                      j   8   <   b   �       �   T   l   @      H   �   -   &   k          
       �   Y       >   G   7   S   3       )       t      a             V   \       D          ~   �       A               Q   i   y           ^              �       �           �   �   z                 o   J       �          F      _   e      m   5   �          �   I       p   X       .   ;   (       �   W       +   M       	       2   g   $   �   u      B   *      E          Admin Autorefresh Available datasets: Back up the database Bucket size Cancel Canceled Capacity: respect capacity limits Change history Cockpit Comments Configure time buckets Confirm Constrained demand Constraints Copy Copy selected objects Create a plan Create a sample model in the database.<br/>
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
 Create new object Create time buckets for reporting.<br/>
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
 Customize Data file Delete selected objects Deliveries Detail Display graph Display table Done Download all input data in a single spreadsheet. Dump the database contents to a file. Edit Edit availability Empty the database Erase selected tables in the database. Error retrieving report data Error: Missing time buckets or bucket dates Export Export a spreadsheet Export as CSV or Excel file Export data to %(erp)s Export frePPLe plan to the ERP. Failed Filter data Forecast method Gantt chart Generate buckets Generate model Import Import CSV or Excel file Import a spreadsheet Import data changes in the last %(delta)s days from the ERP into frePPLe. Import data from %(erp)s Import input data from a spreadsheet.<br/>The spreadsheet must match the structure exported with the task above. Inquiry Keep active in memory Launch Launch new tasks Lead time: do not plan in the past Load a dataset Load a dataset from a file in the database. Log file Material: respect procurement limits More records exists. Only %(limit)s are shown. Move in Move out Pegging Plan Plan detail Quote Read Release Release fence: do not plan within the release time window Release selected scenarios Reset Save changes Scenario management Sorry, You don't have any execute permissions... Stop the web service. Supply Path Time buckets Too many objects to display Undo changes Update Update description of selected scenarios View log file Waiting Web service Where Used Why short or late? Write Zoom in Zoom out after plan current date available backlog buffer comments consumed criticality date days demand description end date end inventory forecast forecast adjustment forecast baseline forecast consumed forecast net forecast total free from identifier into selected scenarios item last refresh load location locked ends locked starts minimum months name new ends new starts open orders orders adjustment overload parameters planned net forecast planned orders problems produced quantity setup start date start inventory status supply to total demand total ends total orders total starts total supply type unavailable units value weeks Project-Id-Version: 3.0.beta
Report-Msgid-Bugs-To: 
POT-Creation-Date: 2015-10-12 10:21+0200
PO-Revision-Date: 2015-09-08 17:29+0200
Last-Translator: Johan De Taeye <jdetaeye@frepple.com>
Language-Team: American English <kde-i18n-doc@kde.org>
Language: en_US
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
X-Poedit-Basepath: /home/frepple/workspace/frepple/contrib/django/freppledb
X-Generator: Poedit 1.8.2
Plural-Forms: nplurals=2; plural=(n != 1);
 Administratie Vernieuw automatisch Beschikbare datasets: Databank backup aanmake Tijdsperiode Annuleer Geannuleerd Capaciteit: respecteer beschikbare capaciteit Historiek Cockpit Commentaar Aanpassen tijdsindeling Bevestig Constrained vraag Constraints Kopieer Kopieer geselecteerde objecten Creëer een plan Creëer een voorbeeldmodel.<br/>
De volgende parameters controleren de grootte en complexiteit.<br/>
Aantal eindproducten: <input id="create0" name="clusters" type="text" maxlength="5" size="5" value="100" onchange="calcUtil()"/><br/>
<b>Vraag:</b><br/>
&nbsp;&nbsp;Maandelijkse forecast per eindproduct: <input id="create1" name="fcst" type="text" maxlength="4" size="4" value="50" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Orders per eindproduct: <input id="create2" name="demands" type="text" maxlength="4" size="4" value="30" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Gemiddelde levertijd: <input id="create3" name="deliver_lt" type="text" maxlength="4" size="4" value="30" onchange="calcUtil()"/> dagen<br/>
<b>Grondstoffen:</b><br/>
&nbsp;&nbsp;Diepte van de bill-of-material: <input id="create4" name="levels" type="text" maxlength="2" size="2" value="5" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Totaal aantal componenten: <input id="create5" name="components" type="text" maxlength="5" size="5" value="200" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Aantal componenten per eindproduct: <input id="create6" name="components_per" type="text" maxlength="5" size="5" value="4" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Gemiddelde levertijd van leveranciers: <input id="create7" name="procure_lt" type="text" maxlength="4" size="4" value="40" onchange="calcUtil()"/> dagen<br/>
<b>Capaciteit:</b><br/>
&nbsp;&nbsp;Aantal machines: <input id="create8" name="rsrc_number" type="text" maxlength="3" size="3" value="60" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Capaciteit van elke machine: <input id="create9" name="rsrc_size" type="text" maxlength="3" size="3" value="5" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Verwachte gemiddelde bezettingsgraad: <span id="util">76.7</span>&#37;<br/>
 Creeer nieuw object Creëer tijdsperiodes voor rapporten.<br/>
Start datum: <input class="date" name="start" type="text" maxlength="5" size="12"/><br/>
End datum: <input class="date" name="end" type="text" maxlength="5" size="12"/><br/>
Week start op: <select name="weekstart">
<option value="0">Zondag</option>
<option value="1" selected="selected">Maandag</option>
<option value="2">Dinsdag</option>
<option value="3">Woensdag</option>
<option value="4">Donderdag</option>
<option value="5">Vrijdag</option>
<option value="6">Zaterdag</option>
</select>
 Personaliseer Databestand Verwijder geselecteerde objecten Leveringen Detail Toon grafiek Toon tabel Klaar Laad alle input data in een enkele spreadsheet. Dump the databank gegevens naar een bestand. Aanpassen Aanpassen beschikbaarheid Databank leegmaken Maak geselecteerde tabellen leeg in de databank Fout tijdens ophalen van rapport data Fout: Geen time buckets Exporteer Exporteer een spreadsheet Exporteer CSV of Excel bestand Exporteer data naar %(erp)s Exporteer frePPLe plan naar het ERP systeem. Error Filter gegevens Forecast methode Gantt chart Genereer tijdshorizon Genereer model Importeer Importeer CSV of Excel bestand Importeer een spreadsheet Importeer gewijzigde data van de laatste %(delta)s dagen uit het ERP systeem in frePPLe. Importeer data van %(erp)s Importeer input data van een spreadsheet<br/>De spreadsheet moet hetzelfde formaat hebben als geëxporteerd met bovenstaande taak. Navraag Houd actief in het geheugen Lanceer Lanceer nieuwe taken Lead time: creëer geen plannen in het verleden Laad een dataset Laad een dataset uit een bestand in de database Log bestand  Materiaal: respecteer aankoopbeperkingen Er zijn meer records. Enkel  %(limit)s worden getoond. Vroeger Later Pegging Plan Plan detail Quote Lees Geef vrij Vrijgave window: creëer geen plannen in het vrijgave window Vrijgave geselecteerde scenarios Herstel Sla wijzigingen op Scenariobeheer Sorry, U heeft geen gebruikersrechten om opdrachten uit te voeren... Stop de webservice. Supply Pad Tijdsindeling Teveel objecten om te tonen Undo wijzigingen Update Update omschrijving van geselecteerde scenarios Bekijk log bestand Wachtend Webservice Waar Gebruikt Waarom te weinig of te laat? Schrijf Zoem in Zoem uit na de huidige datum beschikbaar achterstand buffer commentaar verbruikte hoeveelheid kriticiteit datum dagen order omschrijving einddatum eindvoorraad forecast forecastcorrectie basisforecast geconsumeerde forecast netto forecast  totale forecast vrij van rangnummer in de geselecteerde scenarios product laatste update load locatie bevroren ends bevroren starts minimum maanden naam nieuwe ends nieuwe starts open orders order correctie overbelasting parameters ingeplande netto forecast geplande orders problemen geproduceerde hoeveelheid waarde setup startdatum beginvoorraad status aanbod tot totale vraag totaal ends totale orders totaal starts totale aanvoer type onbeschikbaar aantal waarde weken 