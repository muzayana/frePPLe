��    �      4    L      h  �   i  [    �   ]  �   J  �   �     �     �     �  }   �     S     _     s     �     �     �  !   �     �     �     �     �           	           (     :     M     Y     ^     t  �  �     a    s  	   �  	   �  	   �     �     �     �  q   �  �   =   
   �      �      �      !     !  0   !  %   P!     v!     {!     �!  &   �!     �!  +   �!     "     "     ,"     H"     _"     "     �"     �"     �"     �"     �"     �"     �"     �"     �"     �"  I   #     Z#  p   s#     �#     �#     $     	$  "   $     =$  +   L$     x$     �$  $   �$  .   �$     �$     �$     �$     �$     	%     %     (%  	   C%     M%     U%     Z%     f%      �%  �   �%     /&     5&     :&  9   B&     |&     �&     �&     �&     �&     �&     �&     �&     �&     '     !'  0   4'     e'     {'  �   �'  X   (     f(     s(     �(     �(  (   �(     �(     �(     �(     �(  7   �(     ,)  
   8)     C)     V)     \)     j)     �)     �)     �)  	   �)     �)     �)     �)     �)     �)     �)     �)     �)     �)     *     *     *     "*     6*     H*     Z*     g*     v*     {*  
   �*     �*     �*     �*     �*     �*     �*     �*     �*     �*     �*     �*  
   �*     +     +     #+  
   ,+     7+     L+     [+     d+     m+     v+  
   |+     �+     �+     �+     �+     �+  
   �+     �+     �+     �+     �+     �+     �+     �+     ,    
,  {   .    �.  �   �/  u   U0  �   �0     z1     �1     �1  X   �1     2     2     !2     12     >2     E2     L2     h2     v2     �2     �2     �2     �2     �2     �2     �2     �2     �2     �2     �2  �  �2     }9    �9     �;     �;     �;     �;     �;     �;  �   �;  �   G<     �<     �<     �<     �<     �<     �<  -   =     H=  	   O=     Y=     i=     �=     �=     �=     �=     �=     �=     >     >     %>     2>  	   ?>     I>     V>     c>     p>     w>     ~>     �>  3   �>     �>  _   �>     Q?     X?     e?     l?  '   |?     �?  -   �?     �?     �?     �?  -   @     @@     G@     N@     [@     h@     x@     �@  	   �@     �@     �@     �@     �@     �@  �   �@  	   wA     �A     �A  6   �A     �A     �A     �A     �A     �A     B     B     !B     .B     ;B     ZB  *   oB     �B  	   �B  R   �B  Q   
C  	   \C     fC     sC     �C     �C     �C     �C     �C     �C  *   �C     D     D     D     9D     =D     DD     `D     gD     nD  	   �D     �D     �D     �D     �D     �D     �D     �D     �D     �D     �D     �D     �D     �D     �D     E     E     %E     2E     ?E  	   CE     ME     lE     sE     �E     �E     �E     �E  	   �E     �E     �E  	   �E     �E     �E     �E     �E     �E     F     F     "F     )F     0F     7F     >F     KF     XF     _F     fF     jF     wF     �F     �F     �F     �F  	   �F     �F     �F     �F                8   X       �   v             9   J          �       w          #          _   �   Z   Q       �       ?                  s      n      l      �   .   �   �   �       �      �              �   �   7               5   �   q       f           *   m   �      }   �       B   %   Y   L              �   �   0   |   \   R       �   �   +   �   N       @   �   �   r       ^   �   �   �           C   �   ;       �   �   �   U   	          �   -         W   d   :           �   �   �              �       �   �   '           z       �   �   k   ]   I   P   6       e   p   �   b   K       �   O          �   �   [      ,           u   �   j   /   o   D       �   A              
   <               V   �   �       (       >   {          �       )   G   �   a   c   �   �       �   4   �   t   1      i   �   F           g   3   H   !   $   �         �   �       =   �                     h   S   �   M   &   x   �   �   �       "   `   �   �       T             �       �   �   �   y   ~           E   �   2        
Load frePPLe from the database and live data sources...<br/>
and create a plan in frePPLe...<br/>
and export results.<br/>
<br/>
<b>Plan type</b><br/> <b title="A live data source allows your frePPLe plan to be 100%% in sync with data in an external system.<br/><br/>FrePPLe will read data from them before planning.<br/>And after the plan is generated frePPLe directly exports the results to them.<br/>FrePPLe also saves a copy of the data in its own database for reporting.">Live data sources</b> <b title="The planning engine is normally shut down after generating the plan.<br/><br/>With this option you can keep the plan active in memory as a web service, which is used for order quoting and interactive planning.">Web service</b> <span title="This plan respects the constraints enabled below.<br/>In case of shortages the demand is planned late or short.">Constrained plan</span> <span title="This plan shows material, capacity and operation problems that prevent the demand from being planned in time.<br/>The demand is always met completely and on time.">Unconstrained plan</span> Action Add another %(verbose_name)s Admin Are you sure you want to delete the %(object_name)s "%(escaped_object)s"? All of the following related items will be deleted: Autorefresh Available datasets: Back up the database Bucket size Cancel Canceled Capacity: respect capacity limits Change history Change my password Change password Cockpit Comments Configure time buckets Confirm Confirm password: Constrained demand Constraints Copy Copy selected objects Create a plan Create a sample model in the database.<br/>
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
 Customize Data file Date/time Delete Delete selected objects Delete? Deleting the %(object_name)s '%(escaped_object)s' would require deleting the following protected related objects: Deleting the %(object_name)s '%(escaped_object)s' would result in deleting related objects, but your account doesn't have permission to delete the following types of objects: Deliveries Detail Display graph Display table Done Download all input data in a single spreadsheet. Dump the database contents to a file. Edit Edit availability Empty the database Erase selected tables in the database. Error retrieving report data Error: Missing time buckets or bucket dates Export Export a spreadsheet Export as CSV or Excel file Export data to %(erp)s Export frePPLe plan to the ERP. Failed Filter data Forecast method Gantt chart Generate buckets Generate model History Home Import Import CSV or Excel file Import a spreadsheet Import data changes in the last %(delta)s days from the ERP into frePPLe. Import data from %(erp)s Import input data from a spreadsheet.<br/>The spreadsheet must match the structure exported with the task above. Inquiry Keep active in memory Launch Launch new tasks Lead time: do not plan in the past Load a dataset Load a dataset from a file in the database. Log file Log in Material: respect procurement limits More records exists. Only %(limit)s are shown. Move in Move out New password: Old password: Page not found Password change Password change successful Password: Pegging Plan Plan detail Please correct the error below. Please correct the errors below. Please enter your old password, for security's sake, and then enter your new password twice so we can verify you typed it in correctly. Quote Read Release Release fence: do not plan within the release time window Release selected scenarios Remove Reset Save Save and add another Save and continue editing Save as new Save changes Scenario management Server Error <em>(500)</em> Server error (500) Sorry, You don't have any execute permissions... Stop the web service. Supply Path There's been an error. It's been reported to the site administrators via email and should be fixed shortly. Thanks for your patience. This object doesn't have a change history. It probably wasn't added via this admin site. Time buckets Too many objects to display Undo changes Update Update description of selected scenarios User View log file View on site Waiting We're sorry, but the requested page could not be found. Web service Where Used Why short or late? Write Yes, I'm sure Your password was changed. Zoom in Zoom out after plan current date available backlog buffer comments consumed criticality date days demand description end date end inventory forecast forecast adjustment forecast baseline forecast consumed forecast net forecast total free from identifier into selected scenarios item last refresh load location locked ends locked starts minimum months name new ends new starts open orders orders adjustment overload parameters planned net forecast planned orders problems produced quantity setup start date start inventory status supply to total demand total ends total orders total starts total supply type unavailable units value weeks Project-Id-Version: 2.1.beta
Report-Msgid-Bugs-To: 
POT-Creation-Date: 2015-10-12 10:21+0200
PO-Revision-Date: 2014-11-12 16:53+0800
Last-Translator: Johan De Taeye <jdetaeye@frepple.com>
Language-Team: American English <kde-i18n-doc@kde.org>
Language: en_US
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
X-Poedit-Basepath: /home/frepple/workspace/frepple/contrib/django/freppledb
X-Generator: Poedit 1.5.7
Plural-Forms: nplurals=2; plural=(n != 1);
X-Poedit-SearchPath-0: 
 
从数据库加载frePPLe和激活数据源...<br/>
创建计划...<br/>
输出结果.<br/>
<br/>
<b>计划类型</b><br/> <b title="一个实时数据源允许你的frePPLe计划在外部系统100%%同步.<br/><br/>FrePPLe在计划前从外部系统读数据.<br/>产生计划后,FrePPLe会直接输出结果给外部系统.<br/>FrePPLe同样保存数据副本在自己的数据库">实时数据源</b> <b title="排程后，该计划引擎正常的关闭。<br/><br/>该选项可以保持计划在内存作为网络服务活动,用来询交期和交互规划">网络服务</b> <span title="该计划遵循以下的约束<br/>需求不足的情况下会排程延迟或短期">约束计划</span> <span title="该计划说明物料，产能和工序的问题，防止需求被最后安排。<br/>需求总是可以完全按时安排计划。">没有约束的计划</span> 动作 新增另一个%(verbose_name)s 管理 确定删除 %(object_name)s "%(escaped_object)s"？以下内容将会被全部删除： 自动更新 有效的数据集 备份数据库 日期范围 取消 取消 能力：遵循能力限制 修改历史  修改密码 修改密码 组件 评论 配置计划日期 确认 确认密码： 约束需求 约束 复制 复制 创建计划 在数据库创建一个示例模板。<br/>
这些参数控制规模和复杂性。<br/>
成品的数量: <input id="create0" name="clusters" type="text" maxlength="5" size="5" value="100" onchange="calcUtil()"/><br/>
<b>需求:</b><br/>
&nbsp;&nbsp;每个成品的月预测: <input id="create1" name="fcst" type="text" maxlength="4" size="4" value="50" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;每个成品的需求: <input id="create2" name="demands" type="text" maxlength="4" size="4" value="30" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;平均交货生产周期: <input id="create3" name="deliver_lt" type="text" maxlength="4" size="4" value="30" onchange="calcUtil()"/> 天<br/>
<b>原料:</b><br/>
&nbsp;&nbsp;物料清单的深度: <input id="create4" name="levels" type="text" maxlength="2" size="2" value="5" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;组件总数量: <input id="create5" name="components" type="text" maxlength="5" size="5" value="200" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;成品组件的数量: <input id="create6" name="components_per" type="text" maxlength="5" size="5" value="4" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;平均采购周期: <input id="create7" name="procure_lt" type="text" maxlength="4" size="4" value="40" onchange="calcUtil()"/> 天<br/>
<b>产能:</b><br/>
&nbsp;&nbsp;资源数: <input id="create8" name="rsrc_number" type="text" maxlength="3" size="3" value="60" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;每个资源的规模: <input id="create9" name="rsrc_size" type="text" maxlength="3" size="3" value="5" onchange="calcUtil()"/><br/>
&nbsp;&nbsp;期望平均资源利用: <span id="util">76.7</span>&#37;<br/>
 新建 为报告创建时间段.<br/>
开始日期: <input class="date" name="start" type="text" maxlength="5" size="12"/><br/>
结束日期: <input class="date" name="end" type="text" maxlength="5" size="12"/><br/>
一周开始: <select name="weekstart">
<option value="0">周日</option>
<option value="1" selected="selected">周一</option>
<option value="2">周二</option>
<option value="3">周三</option>
<option value="4">周四</option>
<option value="5">周五</option>
<option value="6">周六</option>
</select>
 定制 数据文件 日期/时间 删除 删除 删除? 删除%(object_name)s '%(escaped_object)s'将引起其它相关的内容被删除，但你没有权限删除以下类型的内容： 删除%(object_name)s '%(escaped_object)s'将引起其它相关的内容被删除，但你没有权限删除以下类型的内容： 交货 明细 显示图形 显示表格 完成 下载所有内容到数据表 将数据库的内容转储到一个文件。 编辑 可编辑 清空数据库 在数据库清除所选的表 错误检索数据报表 错误:没有计划日期。 导出 导出数据表 导出CSV或Excel文件 导出数据到%(erp)s 导出frePPLe计划到ERP 失败 过滤数据 预测方法 甘特图 生成日期 生产模板 历史记录 首页 导入 导入CSV或Excel文件 导入数据表 从ERP导入最新 %(delta)s数据变化到frePPLe. 从%(erp)s导入数据 从数据表中导入数据。<br/>该数据表必须与上述导出任务的结构相匹配。 调查 保持运行 开始 推出新任务 交付周期：不在过去安排计划 加载数据集 从一个文件中加载数据集到数据库 日志文件 登录 物料：遵循采购限制 更多记录存在。只有%(limit)s显示。 移入 移出 新密码： 旧密码： 找不到页面 修改密码 密码修改成功 密码： 分析 计划 计划明细 请纠正以下错误。 请纠正以下错误。 为安全起见，请先输入你的旧密码，然后再输入你的新密码。为了核实你的输入，请输入两次新密码。 询交期 读 释放 释放冻结：不在释放的时间窗口安排计划 释放所选方案 移除 重置 保存 保存和新增另一个 保存和继续编辑 新增 保存修改 方案管理 服务器错误 <em>(500)</em> 服务器错误(500) 对不起，您沒有任何执行权限... 停止网站服务 供应线 发生错误。已经发送给管理员,将会尽快解决。感谢您的耐心。 这个对象没有修改历史。它可能不是通过这个网站来添加的。 时间段 过多项目 撤销修改 更新 更新已选择方案的描述 用户 查看日记文件 在网站查看 等待 对不起，没有找到请求的页面。 网站服务 用于何处 为什么会短缺/延迟？ 写 确定 你的密码已经被修改 放大 缩小 计划当前日期后 有效性 合计 库存 评论 消耗 危急程度 日期 天 需求 描述 结束日期 最终库存 预测 预测调整 预测基线 预测消耗 预测网络 总体预测 不受约束 从 标识符 添加到已选择的方案中 产品 上次更新 负载 地点 锁定结束 锁定开始 最小值 月 名称 新结束 新的开始 未结订单 订单调整 负载过重 参数 计划网络预测 计划订单 问题 生产 数量 配置 开始日期 初始库存 状态 供应 到 总体需求 全部结束 全部订单 全部开始 总体供应 类型 不可用 单元 值 周 