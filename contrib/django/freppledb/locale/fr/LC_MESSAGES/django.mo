��    �      <  �   \
      �  �   �  [  q  �   �  �   �  �   P          8     >     J     ^     s          �  !   �     �     �     �     �     �     �                    *  �  8         )  	   <  	   F     P     W     o  
   w     �     �     �     �  0   �  %   �                 &   +     R  +   o     �     �     �     �     �     
               -     9     J     Y     `     y  I   �     �  p   �     b     j     �     �  "   �     �  +   �     �  $   �  .   $      S      [      d      l      q      }      �      �   9   �      �      �      �      �      �   0   !     D!     Z!  �   f!     �!     �!     "     ""  (   )"     R"     `"     m"     u"  
   �"     �"     �"     �"     �"     �"  	   �"     �"     �"     �"     �"     �"     #     
#     #     #     "#     +#     9#     B#     V#     h#     z#     �#     �#     �#  
   �#     �#     �#     �#     �#     �#     �#     �#     �#     $     $     $  
   $     %$     1$     C$  
   L$     W$     l$     {$     �$     �$     �$  
   �$     �$     �$     �$     �$     �$  
   �$     �$     �$     �$     %     %     %     %     $%  �  *%  �    '  �  �'  &  x)  �   �*  �   @+  !   /,     Q,     `,  "   ~,     �,     �,     �,     �,  -   �,  
   -     -     .-  #   ;-  	   _-     i-     |-     �-      �-     �-    �-     �4  (  �4     7     &7     :7  #   B7     f7  
   r7     }7     �7     �7     �7  ;   �7  -   �7     8     #8     >8  ?   X8  +   �8  0   �8     �8     �8  #   9      A9  "   b9     �9     �9     �9     �9  "   �9     �9     �9      �9      :  _   ?:  "   �:  �   �:     \;     j;     �;     �;  0   �;     �;  ,   �;     $<  4   4<  O   i<     �<     �<     �<     �<     �<     �<     =     =  @   =  &   V=  	   }=     �=     �=     �=  4   �=      >     >  �   5>     �>     �>     �>     ?  ;   #?     _?     s?  
   �?     �?  
   �?  #   �?     �?  
   �?     �?     �?  
   @     @     @     "@     /@     ?@     L@     Q@     W@     _@     k@     w@  
   �@     �@     �@     �@     �@     �@     �@     �@     �@  "   A     (A     0A     GA     NA     ZA     nA     �A     �A     �A     �A     �A     �A     �A  
   �A     �A     �A     B  
   $B     /B  	   =B     GB     MB     \B     lB     sB     �B     �B  	   �B     �B     �B     �B     �B     �B     �B     �B     �B         7   �   �           �   �      n   g   +   V   Y       I   }          $                      2   -       L   y   4                 �   R      .       �   {   �   e   H   c   p       F   )   f       v   s   �       B   @          	   _       �      �               �       K   '   S           d          (   U          �   �              #   r   G   9      :   [   �   �              `   Z      �                   h   �   l   �   P   q   �   =   b   k         �   �      �   C                 �       ,   %   /   �   w   �   �               �      j       ^       o      0   J   >   ]   8          T       Q       �   !   t       �          3          D   |   &   O   ?       1                    �       x   �       6   �   M   5   �   X       z   ~   
           W   m   A      �   u   a   �   ;   E   N       "   *      �       \       i   <       �    
Load frePPLe from the database and live data sources...<br/>
and create a plan in frePPLe...<br/>
and export results.<br/>
<br/>
<b>Plan type</b><br/> <b title="A live data source allows your frePPLe plan to be 100%% in sync with data in an external system.<br/><br/>FrePPLe will read data from them before planning.<br/>And after the plan is generated frePPLe directly exports the results to them.<br/>FrePPLe also saves a copy of the data in its own database for reporting.">Live data sources</b> <b title="The planning engine is normally shut down after generating the plan.<br/><br/>With this option you can keep the plan active in memory as a web service, which is used for order quoting and interactive planning.">Web service</b> <span title="This plan respects the constraints enabled below.<br/>In case of shortages the demand is planned late or short.">Constrained plan</span> <span title="This plan shows material, capacity and operation problems that prevent the demand from being planned in time.<br/>The demand is always met completely and on time.">Unconstrained plan</span> Add another %(verbose_name)s Admin Autorefresh Available datasets: Back up the database Bucket size Cancel Canceled Capacity: respect capacity limits Change history Cockpit Comments Configure time buckets Confirm Constrained demand Constraints Copy Copy selected objects Create a plan Create a sample model in the database.<br/>
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
 Customize Data file Delete Delete selected objects Delete? Deliveries Detail Display graph Display table Done Download all input data in a single spreadsheet. Dump the database contents to a file. Edit Edit availability Empty the database Erase selected tables in the database. Error retrieving report data Error: Missing time buckets or bucket dates Export Export a spreadsheet Export as CSV or Excel file Export data to %(erp)s Export frePPLe plan to the ERP. Failed Filter data Forecast method Gantt chart Generate buckets Generate model Import Import CSV or Excel file Import a spreadsheet Import data changes in the last %(delta)s days from the ERP into frePPLe. Import data from %(erp)s Import input data from a spreadsheet.<br/>The spreadsheet must match the structure exported with the task above. Inquiry Keep active in memory Launch Launch new tasks Lead time: do not plan in the past Load a dataset Load a dataset from a file in the database. Log file Material: respect procurement limits More records exists. Only %(limit)s are shown. Move in Move out Pegging Plan Plan detail Quote Read Release Release fence: do not plan within the release time window Release selected scenarios Remove Reset Save changes Scenario management Sorry, You don't have any execute permissions... Stop the web service. Supply Path There's been an error. It's been reported to the site administrators via email and should be fixed shortly. Thanks for your patience. Time buckets Too many objects to display Undo changes Update Update description of selected scenarios View log file View on site Waiting Web service Where Used Why short or late? Write Zoom in Zoom out after plan current date available backlog buffer comments consumed criticality date days demand description end date end inventory forecast forecast adjustment forecast baseline forecast consumed forecast net forecast total free from identifier into selected scenarios item last refresh load location locked ends locked starts minimum months name new ends new starts open orders orders adjustment overload parameters planned net forecast planned orders problems produced quantity setup start date start inventory status supply to total demand total ends total orders total starts total supply type unavailable units value weeks Project-Id-Version: 3.0.beta
Report-Msgid-Bugs-To: 
POT-Creation-Date: 2015-10-12 10:56+0200
PO-Revision-Date: 2014-11-11 17:45+0100
Last-Translator: Guy Ollagnon <guy.ollagnon@demand2plan.com>
Language-Team: American English <kde-i18n-doc@kde.org>
Language: en_US
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
X-Poedit-Basepath: /home/frepple/workspace/frepple/contrib/django/freppledb
X-Generator: Poedit 1.6.10
Plural-Forms: nplurals=2; plural=(n != 1);
 
Chargement de frePPLe depuis la base de données et les données opérationnelles...<br/>
et créer un plan dans frePPLe...<br/>
et exporter les résultats.<br/>
<br/>
<b>Plan type</b><br/> <b title="Une source de données interactive permet à votre plan frePPLe d'être 100%% synchronisé avec les données des systèmes externes.<br/><br/>FrePPLe va charger les données avant de planifier.<br/>Et après la génération du plan frePPLe exportera directement les données.<br/>FrePPLe enregistre aussi une copie des informations dans sa base de données pour du reporting.">Live data sources</b> <b title="Le moteur de planification est normalement arrêté après la génération du plan.<br/><br/>Avec cette option il est possible de conserver le plan en mémoire comme un service web, pour être utilisé pour qualifier une commandes et réaliser du planning interactif.">Web service</b> <span title="Ce plan respecte les contraintes définies plus bas.<br/>En cas de rupture la demande est planifiée en retard ou manquante.">Plan contraint</span> <span title="Ce plan contient des problèmes de matières, de capacités, et d'opérations, qui empêchent la demande d'être planifiée à temps.<br/>La demande est toujours satisfaite entièrement et à temps.">Plan non contraint</span> Ajouter un autre %(verbose_name)s Administration Rafraîchissement automatique Ensembles de données disponibles: Sauvegarder la base de données Maille temporelle Annuler Annulé  Capacité : respect des limites de capacité Historique Poste de pilotage Commentaires Configure les périodes temporelles Confirmer Demande contrainte Contraintes Copier Copier les objets sélectionnés Créer un plan Créer un modèle.<br/>
Ces paramètres contrôlent la taille et la complexité du modèle.<br/>
Nombre de produits finis: <input id="create0" name="clusters" type="text" maxlength="5" size="5" value="100" onchange="calcUtil()"/><br/>
<b>Demande:</b><br/>
&nbsp;&nbsp;Prévision mensuelle par produit fini: <input id="create1" name="fcst" type="text" maxlength="4" size="4" value="50" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Demand par produit fini: <input id="create2" name="demands" type="text" maxlength="4" size="4" value="30" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Délai de livraison moyen: <input id="create3" name="deliver_lt" type="text" maxlength="4" size="4" value="30" onchange="calcUtil()"/> jours<br/>
<b>Matières premières:</b><br/>
&nbsp;&nbsp;Nombre de niveaux des nomenclatures: <input id="create4" name="levels" type="text" maxlength="2" size="2" value="5" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Nombre total de composants: <input id="create5" name="components" type="text" maxlength="5" size="5" value="200" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Nombre de composants par produit fini: <input id="create6" name="components_per" type="text" maxlength="5" size="5" value="4" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Délai d'approvisionnement moyen: <input id="create7" name="procure_lt" type="text" maxlength="4" size="4" value="40" onchange="calcUtil()"/> jours<br/>
<b>Capacité:</b><br/>
&nbsp;&nbsp;Nombre de ressources: <input id="create8" name="rsrc_number" type="text" maxlength="3" size="3" value="60" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Capacité de chaque ressource: <input id="create9" name="rsrc_size" type="text" maxlength="3" size="3" value="5" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;Utilisation moyenne des ressources attendue: <span id="util">76.7</span>&#37;<br/>
 Créer un nouvel objet Créer les périodes temporelles pour le reporting.<br/>
Date de début: <input class="date" name="start" type="text" maxlength="5" size="12"/><br/>
Date de fin: <input class="date" name="end" type="text" maxlength="5" size="12"/><br/>
Semaine de début: <select name="weekstart">
<option value="0">Dimanche</option>
<option value="1" selected="selected">Lundi</option>
<option value="2">Mardi</option>
<option value="3">Mercredi</option>
<option value="4">Jeudi</option>
<option value="5">Vendredi</option>
<option value="6">Samedi</option>
</select>
 Personnaliser Fichier de données Effacer Supprimer les objets sélectionnés Supprimer ? Livraisons Détail Afficher le graphique Afficher la table Fait Charger toutes les données d'entrées dans un seul tableau Exporter la base de données dans un fichier. Editer Modifier la disponibilité Vider la base de données Suppression des tables sélectionnées dans la base de données Erreur récupérant les données du rapport Erreur : mailles temporelles ou dates manquantes Exporter Exporter une feuille de calcul Exporter comme fichier CSV ou Excel Export des données vers %(erp)s Export du plan frePPLe vers l'ERP. Manqué Filtrer les données Methode de prévision Gantt Creation des périodes temporelles Générer le modèle Importer Importer un fichier CSV ou Excel Importer une feuille de calcul Import des données modifiées au cours des %(delta)s derniers jours depuis l'ERP dans frePPLe. Import des données depuis %(erp)s Importer les données d'entrée depuis une feuille de calcul.<br/>La feuille de calcul doit respecter la structure exportée lors de la tâche ci-dessus. Renseignement Garder actif en mémoire Lancer Lancer les nouvelles tâches  Lead time : pas de planification dans le passé Charger un ensembe de données Charger un ensemble de données d'un fichier Fichier journal  Matières : respect des limites d'approvisionnement D'autres enregistrements existent. Seuls les %(limit)s premiers sont affichés. Période précédente Période suivante Pegging Plan Plan détaillé Citation Lire Libérer  Période gelée : pas de planification dans une période gelée Libérer les scénarios sélectionnés Supprimer Réinitialiser Enregistrer les modifications Gestion des scénarios Désolé, vous n'avez pas les droits d'exécution... Arrêter le service web. Réseau d'approvisionnement Une erreur a eu lieu. Elle a été signalée aux administrateurs du site par email and devrait être corrigée sous peu. Merci de votre patience. Période temporelle Trop d'objets sont à afficher Annuler les modifications Mettre à jour Mettre à jour la description des scénarios sélectionnés Voir le fichier log Consulter sur le sit En attente Service web Where Used Pourquoi une rupture ou un retard ? Enregistrer Zoom avant Zoom arrière après la date actuelle disponible backlog SKU commentaires stock consommé criticalité date jours demande description date de fin stock de fin prévision ajustement de prévision prévision de base prévision consommée prévision nette prévision totale libre de numéro dans les scénarios sélectionnés article dernière mise à jour charge emplacement fin période gelée début période gelée minimum mois nom nouvelle fin nouveau début commandes ouvertes ajustement des commandes sur-charge paramètres demande nette planifiée commandes planifiées problèmes stock produit quantité setup date de début stock de début statut approvisionnement à demande totale fin total commandes totales début global approvisionnement total type non disponible unités valeur semaines 