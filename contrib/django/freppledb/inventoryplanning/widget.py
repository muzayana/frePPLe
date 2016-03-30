#
# Copyright (C) 2016 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from urllib.parse import urlencode

from django.db import DEFAULT_DB_ALIAS, connections
from django.http import HttpResponse
from django.utils.encoding import force_text
from django.utils.translation import ugettext_lazy as _

from freppledb.common.middleware import _thread_locals
from freppledb.common.dashboard import Dashboard, Widget


class StockoutRiskWidget(Widget):
  name = "stockoutrisk"
  title = _("stockout risk")
  tooltip = _("Display how well the current inventory position covers the forecasted demand")
  permissions = (('view_distribution_report', 'Can view distribution report'),)
  asynchronous = True
  url = '/inventoryplanning/drp/?sidx=stockoutrisk&sord=desc'
  green = 10
  yellow = 20

  query = '''
    select location_id,
      count(*),
      sum(case when stockoutrisk <= %s then 1 else 0 end) green,
      sum(case when stockoutrisk > %s and stockoutrisk <= %s then 1 else 0 end) yellow,
      sum(case when stockoutrisk > %s then 1 else 0 end) red
    from out_inventoryplanning
    inner join buffer
      on out_inventoryplanning.buffer_id = buffer.name
    group by location_id
    order by 5 desc
    '''

  javascript = '''
    // Collect the data
    var skucount = 0;
    var data = [];
    var titles = [];

    $("#invAnalysis").next().find("thead th").each(function() {
      titles.push($(this).html());
      });
    $("#invAnalysis").next().find("tbody tr").each(function() {
      var row = [];
      $("td", this).each(function() {
        if (row.length == 0)
          row.push($(this).html());
        else
          row.push(parseInt($(this).html()));
        });
      data.push(row);
      if (row[1] > skucount) skucount = row[1];
      });
    var svgrectangle = document.getElementById("invAnalysis").getBoundingClientRect();
    var x_width = (svgrectangle['width']) / data.length;
    var y = d3.scale.linear().domain([0, skucount]).range([svgrectangle['height'] - 20, 0]);
    var y_zero = y(0);

    // Draw the chart
    var bar = d3.select("#invAnalysis")
     .selectAll("g")
     .data(data)
     .enter()
     .append("g")
     .attr("transform", function(d, i) { return "translate(" + (i * x_width) + ",0)"; })
     .on("mouseover", function(d) {
        $("#invanalysis_tooltip")
          .css("display", "block")
          .html(d[0] + "<br>" + d[2] + " " + titles[2] + "<br>" + d[3] + " " + titles[3] + "<br>" + d[4] + " " + titles[4]);
        })
      .on("mousemove", function(){
        $("#invanalysis_tooltip").css("top", (event.clientY+5) + "px").css("left", (event.clientX+5) + "px");
        })
      .on("mouseout", function(){
        $("#invanalysis_tooltip").css("display", "none")
        });
    // Green SKUs
    bar.append("rect:a")
      .attr("xlink:href", function(d) {
        return url_prefix +
        "/inventoryplanning/drp/?sidx=stockoutrisk&sord=desc&stockoutrisk__lte=%s''' % green + '''&buffer__location__name=" + admin_escape(d[0]);
      }).append("rect")
      .attr("y", function(d) {return y(d[2]) + 10;})
      .attr("height", function(d) {return y_zero - y(d[2]);})
      .attr("rx","2")
      .attr("width", x_width - 2)
      .style("fill", "#828915");


    // Yellow SKUs
    bar.append("rect:a")
      .attr("xlink:href", function(d) {
        return url_prefix +
        "/inventoryplanning/drp/?sidx=stockoutrisk&sord=desc&stockoutrisk__gt=%s''' % green + '''&stockoutrisk__lte=%s''' % yellow + '''&buffer__location__name=" + admin_escape(d[0]);
      }).append("rect")
      .attr("y", function(d) {return y(d[2]+d[3]) + 10;})
      .attr("height", function(d) {return y(d[2]) - y(d[2]+d[3]);})
      .attr("rx","2")
      .attr("width", x_width - 2)
      .style("fill", "#FF9900");

    // Red SKUs
    bar.append("rect:a")
      .attr("xlink:href", function(d) {
        return url_prefix +
        "/inventoryplanning/drp/?sidx=stockoutrisk&sord=desc&stockoutrisk__gt=%s''' % yellow + '''&buffer__location__name=" + admin_escape(d[0]);
      }).append("rect")
      .attr("y", function(d) {return y(d[1]) + 10;})
      .attr("height", function(d) {return y(d[2]+d[3]) - y(d[1]);})
      .attr("rx","2")
      .attr("width", x_width - 2)
      .style("fill", "#D31A00");

    // Location label
    bar.append("text")
      .attr("y", y_zero)
      .attr("x", x_width/2)
      .text(function(d,i) { return d[0]; })
      .style("text-anchor", "end")
      .attr("transform","rotate(90 " + (x_width/2) + " " + y_zero + ")  ");
    '''

  def args(self):
    return "?%s" % urlencode({'green': self.green, 'yellow': self.yellow})

  @classmethod
  def render(cls, request=None):
    green = int(request.GET.get('green', cls.green))
    yellow = int(request.GET.get('yellow', cls.yellow))
    try:
      db = _thread_locals.request.database or DEFAULT_DB_ALIAS
    except:
      db = DEFAULT_DB_ALIAS
    cursor = connections[db].cursor()
    result = [
      '<div id="invanalysis_tooltip" class="tooltip-inner" style="display: none; z-index:10000; position:fixed;"></div>',
      '<svg class="chart" id="invAnalysis" style="width:100%; height: 250px;"></svg>',
      '<table style="display:none"><thead>',
      '<tr><th>location</th><th>total</th>',
        '<th>%s (&lt;= %s%%)</th>' % (force_text(_("green")), green),
        '<th>%s (&gt; %s%% and &lt;= %s%%)</th>' % (force_text(_("yellow")), green, yellow),
        '<th>%s (&gt;= %s%%)</th>' % (force_text(_("red")), yellow),
      '</tr></thead><tbody>' % ()
      ]
    cursor.execute(cls.query, (green, green, yellow, yellow))
    for rec in cursor.fetchall():
      result.append('<tr><td>%s</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td></tr>' % rec)
    result.append('</tbody></table>')
    return HttpResponse('\n'.join(result))

Dashboard.register(StockoutRiskWidget)
