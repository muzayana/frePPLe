#
# Copyright (C) 2015 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

from datetime import datetime
from urllib.parse import urlencode

from django.db import DEFAULT_DB_ALIAS, connections
from django.http import HttpResponse
from django.utils.encoding import force_text
from django.utils.html import escape
from django.utils.http import urlquote
from django.utils.text import capfirst
from django.utils.translation import ugettext_lazy as _

from freppledb.common.dashboard import Dashboard, Widget


class ForecastAccuracyWidget(Widget):
  '''
  Calculate the Symmetric Mean Percentage Error (aka SMAPE)
  for all forecasts.
  The result is aggregated per bucket, weighted by the
  forecast quantity.

  TODO: SQL query contains a hardcoded assumption on monthly time buckets
  '''
  name = "forecast_error"
  title = _("Forecast error")
  tooltip = _("Show the evolution of the SMAPE forecast error")
  permissions = (("view_forecast_report", "Can view forecast report"),)
  asynchronous = True
  history = 12

  def args(self):
    return "?%s" % urlencode({'history': self.history})

  javascript = '''
    var margin_y = 30;  // Width allocated for the Y-axis
    var margin_x = 60;  // Height allocated for the X-axis
    var svg = d3.select("#forecast_error");
    var width = $("#forecast_error").width();
    var height = $("#forecast_error").height();

    // Collect the data
    var domain_x = [];
    var data = [];
    var max_error = 0;
    var min_error = 200;
    $("#forecast_error").next().find("tr").each(function() {
      var nm = $(this).find("td.name").html();
      var val = parseFloat($(this).find("td.val").html());
      domain_x.push(nm);
      data.push( [nm, val] );
      if (val > max_error)
        max_error = val;
      if (val < min_error)
        min_error = val;
      });

    var x = d3.scale.ordinal()
      .domain(domain_x)
      .rangeRoundBands([0, width - margin_y - 10], 0);
    var y = d3.scale.linear()
      .range([height - margin_x - 10, 0])
      .domain([min_error - 5, max_error + 5]);

    // Draw invisible rectangles for the hoverings
    svg.selectAll("g")
     .data(data)
     .enter()
     .append("g")
     .attr("transform", function(d, i) { return "translate(" + ((i) * x.rangeBand() + margin_y) + ",10)"; })
     .append("rect")
      .attr("height", height - 10 - margin_x)
      .attr("width", x.rangeBand())
      .attr("fill-opacity", 0)
      .on("mouseover", function(d) { $("#fcst_tooltip").css("display", "block").html(d[0] + " " + d[1] + "%") })
      .on("mousemove", function(){ $("#fcst_tooltip").css("top", (event.pageY-10)+"px").css("left",(event.pageX+10)+"px"); })
      .on("mouseout", function(){ $("#fcst_tooltip").css("display", "none") });

    // Draw x-axis
    var xAxis = d3.svg.axis().scale(x)
        .orient("bottom").ticks(5);
    svg.append("g")
      .attr("transform", "translate(" + margin_y  + ", " + (height - margin_x) +" )")
      .attr("class", "x axis")
      .call(xAxis)
      .selectAll("text")
      .style("text-anchor", "end")
      .attr("dx", "-.75em")
      .attr("dy", "-.25em")
      .attr("transform", "rotate(-90)" );

    // Draw y-axis
    var yAxis = d3.svg.axis().scale(y)
        .orient("left")
        .ticks(5)
        .tickFormat(d3.format(".0f%"));
    svg.append("g")
      .attr("transform", "translate(" + margin_y + ", 10 )")
      .attr("class", "y axis")
      .call(yAxis);

    // Draw the line
    var line = d3.svg.line()
      .x(function(d) { return x(d[0]) + x.rangeBand() / 2; })
      .y(function(d) { return y(d[1]); });

    svg.append("svg:path")
      .attr("transform", "translate(" + margin_y + ", 10 )")
      .attr('class', 'graphline')
      .attr("stroke","#8BBA00")
      .attr("d", line(data));
    '''

  @classmethod
  def render(cls, request=None):
    cursor = connections[request.database].cursor()
    try:
      cursor.execute("SELECT value FROM common_parameter where name='currentdate'")
      curdate = datetime.strptime(cursor.fetchone()[0], "%Y-%m-%d %H:%M:%S")
    except:
      curdate = datetime.now().replace(microsecond=0)
    history = int(request.GET.get('history', cls.history))
    result = [
      '<svg class="chart" id="forecast_error" style="width:100%; height: 100%"></svg>',
      '<table style="display:none">'
      ]
    query = '''
      select common_bucketdetail.name, 100 * sum( fcst * abs(fcst - orders) / abs(fcst + orders) * 2) / greatest(sum(fcst),1)
      from
        (
        select
          startdate,
          greatest(coalesce(forecastadjustment,forecastbaseline),0) fcst,
          greatest(orderstotal + coalesce(ordersadjustment,0),0) orders
        from forecastplan
        where startdate < %s and startdate > %s - interval '%s month'
        ) recs
      inner join common_bucketdetail
        on common_bucketdetail.bucket_id = 'month' and common_bucketdetail.startdate = recs.startdate
      where fcst > 0 or orders > 0
      group by common_bucketdetail.name, recs.startdate
      order by recs.startdate
      '''
    cursor.execute(query, (curdate, curdate, history))
    for res in cursor.fetchall():
      result.append('<tr><td class="name">%s</td><td class="val">%.1f</td></tr>' % (
        escape(res[0]), res[1]
        ))
    result.append('</table>')
    result.append('<div id="fcst_tooltip" style="display: none; z-index:10; position:absolute; color:black">etrere</div>')
    return HttpResponse('\n'.join(result))

Dashboard.register(ForecastAccuracyWidget)
