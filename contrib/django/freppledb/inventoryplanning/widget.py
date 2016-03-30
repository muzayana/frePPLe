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
          .html("<strong>" + d[0] + "</strong><br>" + d[2] + " " + titles[2] + "<br>" + d[3] + " " + titles[3] + "<br>" + d[4] + " " + titles[4]);
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


class InventoryByLocationWidget(Widget):
  name = "inventory_by_location"
  title = _("Inventory by location")
  tooltip = _("Display the locations with the highest inventory value")
  asynchronous = True
  limit = 5

  def args(self):
    return "?%s" % urlencode({'limit': self.limit})

  javascript = '''
    var margin = 50;  // Space allocated for the Y-axis

    // Collect the data
    var invmax = 0;
    var data = [];

    var data = [];
    $("#invByLoc").next().find("tbody tr").each(function() {
      var row = [];
      $(this).find("td").each(function() {
        if (row.length == 0)
          row.push($(this).html());
        else
          row.push(parseFloat($(this).html()));
        });
      data.push(row);
      invmax = (row[3] > invmax) ? row[3] : invmax;
      });

    var svgrectangle = document.getElementById("invByLoc").getBoundingClientRect();
    var x_width = (svgrectangle['width']-margin) / data.length;
    var y = d3.scale.linear().domain([0, invmax]).range([svgrectangle['height'] - 20, 0]);
    var y_zero = y(0);
    var color="";

    // Draw the chart
    var bar = d3.select("#invByLoc")
     .selectAll("g")
     .data(data)
     .enter()
     .append("g")
     .attr("transform", function(d, i) { return "translate(" + (i * x_width + margin) + ",0)"; })
          .on("mouseover", function(d) {
        color=(d[1] >= d[3]) || (d[1] <= d[2]) ? "#D31A00" : ((d[3]-d[1])/d[3]*100 <= 10 || (d[1]-d[2])/d[1]*100 <=10) ? "#FF9900" : "#828915";
        $("#invByLoc_tooltip")
          .css("display", "block")
          .html("<strong>" + d[0] + "</strong><br><table style='background: black'><tr><td>"+ gettext("max on hand")+"</td><td style='text-align: right'>&nbsp;" + d[3] + "</td></tr><tr><td style='color: " + color + ";' ><strong>"+ gettext("on hand")+"</strong></td><td style='text-align: right'><strong>"+ d[1] + "</strong></td></tr><tr><td>" + gettext("safety stock")+"</td><td style='text-align: right'>"+ d[2]) +"</td></tr></table>";
        })
      .on("mousemove", function(){
        $("#invByLoc_tooltip").css("top", (event.clientY+5) + "px").css("left", (event.clientX+5) + "px");
        })
      .on("mouseout", function(){
        $("#invByLoc_tooltip").css("display", "none")
        });

    bar.append("rect:a").attr("xlink:href", function(d) {
        return url_prefix +
        "/buffer/?&location__name=" + admin_escape(d[0]);
      })
      .append("rect")
      .attr("y", function(d) {return y(d[1]) + 10;})
      .attr("height", function(d) {return y_zero - y(d[1]);})
      .attr("rx","3")
      .attr("width", x_width - 2)
      .style("fill",   function(d) {
          return (
              (d[1] >= d[3]) || (d[1] <= d[2]) ? "#D31A00" :
              ((d[3]-d[1])/d[3]*100 <= 10 || (d[1]-d[2])/d[1]*100 <=10) ? "#FF9900" :
              "#828915"
            );
        });

    // Location label
    bar.append("text")
      .attr("y", y_zero)
      .attr("x", x_width/2)
      .text(function(d,i) { return d[0]; })
      .style("text-anchor", "end")
      .attr("transform","rotate(90 " + (x_width/2) + " " + y_zero + ")  ");

    bar.append("line")
      .attr("x1", x_width/2-3)
      .attr("y1", function(d) {return y(d[2]) + 10;})
      .attr("x2", x_width/2-3)
      .attr("y2", function(d) {return y(d[3]) +10;})
      .attr('stroke-width', 2)
      .style("stroke", "black");

    // Draw the Y-axis
    var yAxis = d3.svg.axis()
      .scale(y)
      .ticks(5)
      .orient("left")
      .tickFormat(d3.format("s"));
    d3.select("#invByLoc")
      .append("g")
      .attr("transform", "translate(" + margin + ", 10 )")
      .attr("class", "y axis")
      .call(yAxis);
    '''

  query = '''select location_id, coalesce(sum(buffer.onhand * item.price),0),
                  coalesce(sum(safetystockvalue),0), coalesce(sum(reorderquantityvalue),0)
             from buffer
             inner join item on buffer.item_id = item.name
             inner join out_inventoryplanning
               on out_inventoryplanning.buffer_id = buffer.name
             group by location_id
             order by 2 desc'''

  @classmethod
  def render(cls, request=None):
    limit = int(request.GET.get('limit', cls.limit))

    cursor = connections[request.database].cursor()
    result = [
      '<div id="invByLoc_tooltip" class="tooltip-inner" style="display: none; z-index:10000; position:fixed;"></div>',
      '<svg class="chart" id="invByLoc" style="width:100%; height: 250px;"></svg>',

      '<table style="display:none">',

      ]
    cursor.execute(cls.query)
    for res in cursor.fetchall():
      limit -= 1
      if limit < 0:
        break
      result.append('<tr><td>%s</td><td>%.2f</td><td>%.2f</td><td>%.2f</td></tr>' % (res[0],res[1],res[2],res[2]+res[3]),)
    result.append('</table>')
    return HttpResponse('\n'.join(result))

Dashboard.register(InventoryByLocationWidget)
