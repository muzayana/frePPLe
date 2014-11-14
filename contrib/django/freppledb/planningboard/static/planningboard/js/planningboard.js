
var socket = null;
var curState = 'closed';    // Possible states: closed, connecting, open, disconnecting
var timeAxis = null;

function connect(url, callback)
{
  if (curState != "closed") return;
  curState = "connecting";
  try
  {
    socket = new WebSocket(url);
    socket.onopen = function (ev) {
      curState= "open";
      if (typeof callback !== 'undefined') callback();
      };
    socket.onmessage = onmessage;
    socket.onclose = function (ev) {
      if( curState == 'connecting' )
        console.log("Websocket error: can't talk to server.");
      curState = 'closed';
      };
  }
  catch (ex)
  {
    console.log("Websocket error: "+ ex);
  }
}


function send(data)
{
  if (socket != null)
  {
    try { socket.send(data); }
    catch (ex) { console.log(ex); }
  }
}


function disconnect()
{
  if( curState == "open")
  {
    curState="closing";
    socket.close( 1000, "User Quit");
  }
}


function customize()
{
  $("#entities").dialog({
       title: gettext("Customize"),
       width: 465,
       height: 'auto',
       autoOpen: true,
       resizable: false,
       buttons: [{
         text: gettext("OK"),
         click: function() {
           var new_ganttRows = {};
           numRows = 0;
           $("#entities option:selected").each(function() {
             send("/register/" + this.value);
             if (this.value in ganttRows)
             {
               if (ganttRows[this.value].svg !== null)
                 // Move existing row to a new position
                 ganttRows[this.value].svg.attr("transform", "translate(0," + (numRows*rowheight + timescaleheight) + ")");
               new_ganttRows[this.value] = {"index": numRows++, "svg": ganttRows[this.value].svg};
               delete ganttRows[this.value];
             }
             else
             {
               // Ask the plan for new entities
               new_ganttRows[this.value] = {"index": numRows++, "svg": null};
               send("/plan/" + this.value);
             }
           });
           for (var i in ganttRows)
           {
             // Unregister unselected entities
             send("/unregister/" + i);
             if (ganttRows[i].svg !== null)
               ganttRows[i].svg.remove();
           }
           ganttRows = new_ganttRows;
           $(this).dialog("close");
           }
         },
         {
         text: gettext("Cancel"),
         click: function() { $(this).dialog("close"); }
         }]
       });
    $("#entities").children().first().multiselect({
      collapsableGroups: false,
      sortable: true,
      showEmptyGroups: true,
      locale: $("html")[0].lang,
      searchField: false
      });
}


function onmessage(ev)
{
  xmldoc = $.parseXML(ev.data);
  type = $(xmldoc).find('plan').attr('category');

  // Debugging message
  //console.log(ev.data);

  // Dispatch the message to a handler function
  if (type == "name")
    displayList(xmldoc);
  else if (type=="plan")
    displayPlan(xmldoc);
}


function displayList(xmldoc)  // TODO escaping of the data fields!
{
  var el = $("#demandlist");
  el.html("");
  $(xmldoc).find('demand').each(function() {
    var nm = $(this).attr('name');
    var qty = parseFloat($(this).find('quantity').text());
    var due = new Date(Date.parse($(this).find('due').text()));
    var prio = parseFloat($(this).find('priority').text());
    el.append('&nbsp;&nbsp;' +
      '<span onclick="send(\'/solve/unplan/' + nm + '\')" title="Unplan the demand" class="fa fa-stop"></span>' +
      '&nbsp;&nbsp;<span onclick="send(\'/solve/demand/backward/' + nm + '\')" title="Plan backward from the due date" class="fa fa-backward"></span>' +
      '&nbsp;&nbsp;<span onclick="send(\'/solve/demand/forward/' + nm + '\')" title="Plan forward from the current date" class="fa fa-forward"></span>&nbsp;&nbsp;' +
      nm + "&nbsp;&nbsp;&nbsp;&nbsp;" + due + "&nbsp;&nbsp;&nbsp;&nbsp;" + prio + "&nbsp;&nbsp;&nbsp;&nbsp;" + qty +'<br/>'
      );
  });

  el = $("#resourcelist");
  el.empty();
  $(xmldoc).find('resource').each(function() {
    el.append('<option value="resource/' + $(this).attr('name') + '">' + $(this).attr('name') + '</option>');
  });
  el = $("#bufferlist");
  el.empty();
  $(xmldoc).find('buffer').each(function() {
    el.append('<option value="buffer/' + $(this).attr('name') + '">' + $(this).attr('name') + '</option>');
  });
  el = $("#operationlist");
  el.empty();
  $(xmldoc).find('operation').each(function() {
    el.append('<option value="operation/' + $(this).attr('name') + '">' + $(this).attr('name') + '</option>');
  });
}


function displayPlan(xmldoc)
{
  width = $("#content-main").width() - 24;
  height = numRows * rowheight + timescaleheight;
  $('#ganttdiv').resizable('option', 'maxHeight', height + 10);
  svg = d3.select("#gantt")
    .attr("width", width)
    .attr("height", height);

  // Create a scale for the x-axis and the y-axis
  x = d3.time.scale()
    .domain([horizonstart, horizonend])
    .range([0, width - 250]);

  // Dragging function
  drag = d3.behavior.drag()
    .origin(function(d) { return d; })
    .on("drag", dragmove);

  // Display the operations
  $(xmldoc).find('operations').children().each(function() {
    displayOperation(this);
  });

  // Display the resources
  $(xmldoc).find('resources').children().each(function() {
    displayResource(this);
  });

  // Display the buffers
  $(xmldoc).find('buffers').children().each(function() {
    displayBuffer(this);
    });

  // Display the demands
  $(xmldoc).find('demands').children().each(function() {
    displayDemand(this);
    });
}


function dragmove(d)
{
  if (d3.event.dx < 0)
    d3.select(this).attr("x", d3.select(this).attr("x") - 1);
  else if (d3.event.dx > 0)
    d3.select(this).attr("x", d3.select(this).attr("x") + 1);
}


function displayOperation(xml)
{
  // Look up the row to display the information at
  var res = $(xml).attr('name');
  var indx = ganttRows['operation/' + res].index;
  if (indx === undefined)
    return; // Operation not to be shown at all

  // Parse XML data
  var data = [];
  var layer = [];
  $(xml).find('operationplan').each(function() {
    var row = 0;
    var strt = new Date(Date.parse($(this).find('start').text()));
    var nd = new Date(Date.parse($(this).find('end').text()))
    for (; row < layer.length; ++row)
    {
      if (strt >= layer[row])
      {
         layer[row] = nd;
         break;
      }
    };
    if (row >= layer.length)
      layer.push(nd);
    data.push([
      res,
      strt,
      nd,
      parseFloat($(this).find('quantity').text()),
      row
      ]);
    });

  // Find existing svg row or create a new one
  if (ganttRows['operation/' + res].svg !== null)
  {
    var mysvg = ganttRows['operation/' + res].svg;
    mysvg.selectAll("*").remove();
  }
  else
  {
    var mysvg = svg.append("g")
      .attr("transform", "translate(0," + (indx*rowheight + timescaleheight) + ")");
    ganttRows['operation/' + res].svg = mysvg;
  }

  // Draw Gantt chart
  mysvg.append("line")
    .attr("width", width)
    .attr("height", 200)
    .attr("x1", 0)
    .attr("x2", width)
    .attr("y1", rowheight)
    .attr("y2", rowheight)
    .style("stroke-width", "1")
    .style("stroke", "rgb(0,0,0)")
    .style("fill", "none");
  mysvg.append("text")
    .attr("x", "5")
    .attr("y", rowheight/2)
    .attr("dy", ".35em")
    .text(res)
    .attr('class', 'ganttlabel');
  var h = (rowheight - layer.length * 2 - 2) / Math.max(1, layer.length);
  mysvg.append("g")
    .attr("transform", "translate(250,0)")
    .selectAll("rect")
    .data(data)
    .enter()
    .append("rect")
    .attr("width", function(d) {return x(d[2]) - x(d[1]);})
    .attr("height", h)
    .attr("x", function(d) {return x(d[1]);})
    .attr("y", function(d) {return 2 + d[4] * (h+2);})
    .style("fill","#2B95EC")
    .on("mouseenter", function(d) {
      graph.showTooltip(
        d[0] + '<br/>'
        + $.datepicker.formatDate("yy/mm/dd", d[1]) + " " + d[1].getHours() + ":" + d[1].getMinutes() + ":" + d[1].getSeconds() + ' - '
        + $.datepicker.formatDate("yy/mm/dd", d[2]) + " " + d[2].getHours() + ":" + d[2].getMinutes() + ":" + d[2].getSeconds() + '<br/>'
        + d[3]
        )
      })
    .on("mouseleave", graph.hideTooltip)
    .on("mousemove", graph.moveTooltip)
    .call(drag);
}


function displayResource(xml, indx)
{
  // Look up the row to display the information at
  var res = $(xml).attr('name');
  var indx = ganttRows['resource/' + res].index;
  if (indx === undefined)
    return; // Resource not to be shown at all

  // Parse XML data
  var data = [];
  var layer = [];
  $(xml).find('loadplan').each(function() {
    if ($(this).find('quantity').text().indexOf("-") < 0) {
      var row = 0;
      var strt = new Date(Date.parse($(this).find('start').text()));
      var nd = new Date(Date.parse($(this).find('end').text()))
      for (; row < layer.length; ++row)
      {
        if (strt >= layer[row])
        {
           layer[row] = nd;
           break;
        }
      };
      if (row >= layer.length)
        layer.push(nd);
      data.push([
        $(xml).find('operationplan').attr('operation'),
        strt,
        nd,
        parseFloat($(this).find('quantity').text()),
        row
        ]);
      }
    });

  // Find existing svg row or create a new one
  if (ganttRows['resource/' + res].svg !== null)
  {
    var mysvg = ganttRows['resource/' + res].svg;
    mysvg.selectAll("*").remove();
  }
  else
  {
    var mysvg = svg.append("g")
      .attr("transform", "translate(0," + (indx*rowheight + timescaleheight) + ")");
    ganttRows['resource/' + res].svg = mysvg;
  }

  // Draw Gantt chart
  mysvg.append("line")
    .attr("width", width)
    .attr("height", 200)
    .attr("x1", 0)
    .attr("x2", width)
    .attr("y1", rowheight)
    .attr("y2", rowheight)
    .style("stroke-width", "1")
    .style("stroke", "rgb(0,0,0)")
    .style("fill", "none");
  mysvg.append("text")
    .attr("x", "5")
    .attr("y", rowheight/2)
    .attr("dy", ".35em")
    .text(res)
    .attr('class', 'ganttlabel');
  var h = (rowheight - layer.length * 2 - 2) / Math.max(1, layer.length);
  mysvg.append("g")
  .attr("transform", "translate(250,0)")
  .selectAll("rect")
  .data(data)
  .enter()
  .append("rect")
  .attr("width", function(d) {return x(d[2]) - x(d[1]);})
  .attr("height", h)
  .attr("x", function(d) {return x(d[1]);})
  .attr("y", function(d) {return 2 + d[4] * (h+2);})
  .style("fill","#2B95EC")
  .on("mouseenter", function(d) {
    graph.showTooltip(
      d[0] + '<br/>'
      + $.datepicker.formatDate("yy/mm/dd", d[1]) + " " + d[1].getHours() + ":" + d[1].getMinutes() + ":" + d[1].getSeconds() + ' - '
      + $.datepicker.formatDate("yy/mm/dd", d[2]) + " " + d[2].getHours() + ":" + d[2].getMinutes() + ":" + d[2].getSeconds() + '<br/>'
      + d[3]
      )
    })
  .on("mouseleave", graph.hideTooltip)
  .on("mousemove", graph.moveTooltip)
  .call(drag);
}


function displayBuffer(xml, indx)
{
  // Look up the row to display the information at
  var res = $(xml).attr('name');
  var indx = ganttRows['buffer/' + res].index;
  if (indx === undefined)
    return; // Buffer not to be shown at all

  // Parse XML data
  var data = [];
  var min_oh = 0;
  var max_oh = 0;
  $(xml).find('flowplan').each(function() {
    var oh = parseFloat($(this).find('onhand').text());
    data.push([
      new Date(Date.parse($(this).find('date').text())),
      parseFloat($(this).find('quantity').text()),
      oh,
      parseFloat($(this).find('minimum').text()),
      parseFloat($(this).find('maximum').text())
      ]);
    if (oh < min_oh)
      min_oh = oh;
    if (oh > max_oh)
      max_oh = oh;
    });

  // Find existing svg row or create a new one
  if (ganttRows['buffer/' + res].svg !== null)
  {
    var mysvg = ganttRows['buffer/' + res].svg;
    mysvg.selectAll("*").remove();
  }
  else
  {
    var mysvg = svg.append("g")
      .attr("transform", "translate(0," + (indx*rowheight + timescaleheight) + ")");
    ganttRows['buffer/' + res].svg = mysvg;
  }

  // Draw Gantt chart
  mysvg.append("line")
    .attr("width", width)
    .attr("height", 200)
    .attr("x1", 0)
    .attr("x2", width)
    .attr("y1", rowheight)
    .attr("y2", rowheight)
    .style("stroke-width", "1")
    .style("stroke", "rgb(0,0,0)")
    .style("fill", "none");
  mysvg.append("text")
    .attr("x", "5")
    .attr("y", rowheight/2)
    .attr("dy", ".35em")
    .text(res)
    .attr('class', 'ganttlabel');
  y.domain([min_oh, max_oh]);

  // Draw the inventory profile
  mysvg.append("g")
  .attr("transform", "translate(250,0)")
  .selectAll("g")
  .data(data)
  .enter()
  .append("g")
  .each(function(d, i) {
      var nd = d3.select(this);
      var xpos = x(d[0]);
      if (i > 0)
      {
        var prevx = x(data[i-1][0]);
        if (prevx < 0)
          prevx = 0;
        var prevy = y(data[i-1][2]);
        nd.append("line")
          .attr("x1", prevx)
          .attr("y1", prevy)
          .attr("x2", xpos)
          .attr("y2", prevy)
          .attr("class","inventoryline");
        nd.append("line")
          .attr("x1", xpos)
          .attr("y1", prevy)
          .attr("x2", xpos)
          .attr("y2", y(d[2]))
          .attr("class","inventoryline");
      }
      nd.append("circle")
      .attr("r", 3)
      .attr("cx", xpos)
      .attr("cy", y(d[2]))
      .style("fill", function(d) {if (d[1] > 0) return "#2B95EC"; else return "#F6BD0F";});
    })
  .on("mouseenter", function(d) {
    graph.showTooltip(
      "Quantity: " + d[1] + '<br/>'
      + "Date: " + $.datepicker.formatDate("yy/mm/dd", d[0]) + " " + d[0].getHours() + ":" + d[0].getMinutes() + ":" + d[0].getSeconds() + '<br/>'
      + "On hand: " + d[2]
      )
    })
  .on("mouseleave", graph.hideTooltip)
  .on("mousemove", graph.moveTooltip);
}


function displayDemand(xml)
{
  // TODO Use D3 list of elements. Refresh a single one of them. Or use DC?

  // Update list information
  var res = $(xml).attr('name');
  var qty = parseFloat($(xml).find('quantity').text());
  var due = new Date(Date.parse($(xml).find('due').text()));
  var prio = parseFloat($(xml).find('priority').text());
  $("#demandlist").append('&nbsp;&nbsp;' +
    '<span onclick="send(\'/solve/unplan/' + res + '\')" title="Unplan the demand" class="fa fa-stop"></span>' +
    '&nbsp;&nbsp;<span onclick="send(\'/solve/demand/backward/' + res + '\')" title="Plan backward from the due date" class="fa fa-backward"></span>' +
    '&nbsp;&nbsp;<span onclick="send(\'/solve/demand/forward/' + res + '\')" title="Plan forward from the current date" class="fa fa-forward"></span>&nbsp;&nbsp;' +
    res + "&nbsp;&nbsp;&nbsp;&nbsp;" + due + "&nbsp;&nbsp;&nbsp;&nbsp;" + prio + "&nbsp;&nbsp;&nbsp;&nbsp;" + qty +'<br/>'
    );

  // Look up the row to display the information at
  //var indx = ganttRows['demand/' + res].index;
  //if (indx === undefined)
  //  return; // Buffer not to be shown at all
}

function drawAxis()
{
  timeAxis.selectAll("*").remove();
  var width = $("#content-main").width() - 24 - 250;
  timeAxis.append("line")
    .attr("x1", 0)
    .attr("x2", width)
    .attr("y1", timescaleheight/2)
    .attr("y2", timescaleheight/2)
    .style("stroke-width", "1")
    .style("stroke", "rgb(0,0,0)")
    .style("fill", "none");
  timeAxis.append("line")
    .attr("x1", 0)
    .attr("x2", width)
    .attr("y1", timescaleheight)
    .attr("y2", timescaleheight)
    .style("stroke-width", "1")
    .style("stroke", "rgb(0,0,0)")
    .style("fill", "none");

  // "scaling" stores the number of pixels available to show a day.
  var scaling = 86400000 / (viewend.getTime() - viewstart.getTime()) * width;
  var x = 0;
  if (scaling < 5)
  {
    // Quarterly + monthly buckets
    var bucketstart = new Date(viewstart.getFullYear(), viewstart.getMonth(), 1);
    while (bucketstart < viewend)
    {
      var x1 = (bucketstart.getTime() - viewstart.getTime()) / 86400000 * scaling;
      var bucketend = new Date(bucketstart.getFullYear(), bucketstart.getMonth()+1, 1);
      var x2 = (bucketend.getTime() - viewstart.getTime()) / 86400000 * scaling;
      timeAxis.append('text')
        .attr('class', 'svgheadertext')
        .attr('x', Math.floor((x1+x2)/2))
        .attr('y', timescaleheight-3)
        .text($.datepicker.formatDate("M", bucketstart));
      if (bucketstart.getMonth() % 3 == 0)
      {
        var quarterend = new Date(bucketstart.getFullYear(), bucketstart.getMonth()+3, 1);
        x2 = (quarterend.getTime() - viewstart.getTime()) / 86400000 * scaling;
        var quarter = Math.floor((bucketstart.getMonth()+3)/3);
        timeAxis.append('line')
          .attr('class', 'time')
          .attr('x1', Math.floor(x1))
          .attr('y1', 0)
          .attr('x2', Math.floor(x1))
          .attr('y2', timescaleheight);
        timeAxis.append('text')
          .attr('class', 'svgheadertext')
          .attr('x', Math.floor((x1+x2)/2))
          .attr('y', timescaleheight/2-1)
          .text(bucketstart.getFullYear() + " Q" + quarter);
      }
      else
      {
        timeAxis.append('line')
          .attr('class', 'time')
          .attr('x1', Math.floor(x1))
          .attr('y1', timescaleheight/2)
          .attr('x2', Math.floor(x1))
          .attr('y2', timescaleheight);
      }
      bucketstart = bucketend;
    }
  }
  else if (scaling < 10)
  {
    // Monthly + weekly buckets, short style
    x -= viewstart.getDay() * scaling;
    var bucketstart = new Date(viewstart.getTime() - 86400000 * viewstart.getDay());
    while (bucketstart < viewend)
    {
      timeAxis.append('text')
        .attr('class', 'svgheadertext')
        .attr('x', Math.floor(x + scaling*3.5))
        .attr('y', timescaleheight-3)
        .text($.datepicker.formatDate("mm-dd", bucketstart));
      timeAxis.append('line')
        .attr('class', 'time')
        .attr('x1', Math.floor(x))
        .attr('y1', timescaleheight/2)
        .attr('x2', Math.floor(x))
        .attr('y2', timescaleheight);
      x = x + scaling*7;
      bucketstart.setTime(bucketstart.getTime() + 86400000 * 7);
    }
    bucketstart = new Date(viewstart.getFullYear(), viewstart.getMonth(), 1);
    while (bucketstart < viewend)
    {
      x1 = (bucketstart.getTime() - viewstart.getTime()) / 86400000 * scaling;
      bucketend = new Date(bucketstart.getFullYear(), bucketstart.getMonth()+1, 1);
      x2 = (bucketend.getTime() - viewstart.getTime()) / 86400000 * scaling;
      timeAxis.append('text')
        .attr('class', 'svgheadertext')
        .attr('x', Math.floor((x1+x2)/2))
        .attr('y', timescaleheight/2-1)
        .text($.datepicker.formatDate("M yy", bucketstart));
      timeAxis.append('line')
        .attr('class', 'time')
        .attr('x1', Math.floor(x1))
        .attr('y1', 0)
        .attr('x2', Math.floor(x1))
        .attr('y2', timescaleheight/2);
      bucketstart = bucketend;
    }
  }
  else if (scaling < 20)
  {
    // Monthly + weekly buckets, long style
    x -= viewstart.getDay() * scaling;
    var bucketstart = new Date(viewstart.getTime() - 86400000 * viewstart.getDay());
    while (bucketstart < viewend)
    {
      timeAxis.append('line')
        .attr('class', 'time')
        .attr('x1', Math.floor(x))
        .attr('y1', timescaleheight/2)
        .attr('x2', Math.floor(x))
        .attr('y2', timescaleheight);
      timeAxis.append('text')
        .attr('class', 'svgheadertext')
        .attr('x', x + scaling*7.0/2.0)
        .attr('y', timescaleheight-3)
        .text($.datepicker.formatDate("yy-mm-dd", bucketstart));
      x = x + scaling*7.0;
      bucketstart.setTime(bucketstart.getTime() + 86400000 * 7);
    }
    bucketstart = new Date(viewstart.getFullYear(), viewstart.getMonth(), 1);
    while (bucketstart < viewend)
    {
      x1 = (bucketstart.getTime() - viewstart.getTime()) / 86400000 * scaling;
      bucketend = new Date(bucketstart.getFullYear(), bucketstart.getMonth()+1, 1);
      x2 = (bucketend.getTime() - viewstart.getTime()) / 86400000 * scaling;
      timeAxis.append('line')
        .attr('class', 'time')
        .attr('x1', Math.floor(x1))
        .attr('y1', 0)
        .attr('x2', Math.floor(x1))
        .attr('y2', timescaleheight/2);
      timeAxis.append('text')
        .attr('class', 'svgheadertext')
        .attr('x', Math.floor((x1+x2)/2))
        .attr('y', timescaleheight/2-1)
        .text($.datepicker.formatDate("M yy", bucketstart));
      bucketstart = bucketend;
    }
  }
  else if (scaling <= 40)
  {
    // Weekly + daily buckets, short style
    var bucketstart = new Date(viewstart.getTime());
    while (bucketstart < viewend)
    {
      if (bucketstart.getDay() == 0)
      {
        timeAxis.append('line')
          .attr('class', 'time')
          .attr('x1', Math.floor(x))
          .attr('y1', timescaleheight/2)
          .attr('x2', Math.floor(x))
          .attr('y2', timescaleheight);
        timeAxis.append('text')
          .attr('class', 'svgheadertext')
          .attr('x', Math.floor(x + scaling*7/2))
          .attr('y', timescaleheight/2-1)
          .text($.datepicker.formatDate("yy-mm-dd", bucketstart));
      }
      else
      {
        timeAxis.append('line')
          .attr('class', 'time')
          .attr('x1', Math.floor(x))
          .attr('y1', timescaleheight/2)
          .attr('x2', Math.floor(x))
          .attr('y2', timescaleheight);
      }
      timeAxis.append('text')
        .attr('class', 'svgheadertext')
        .attr('x', Math.floor(x + scaling/2))
        .attr('y', timescaleheight-3)
        .text($.datepicker.formatDate("d", bucketstart));
      x = x + scaling;
      bucketstart.setDate(bucketstart.getDate()+1);
    }
  }
  else if (scaling <= 75)
  {
    // Weekly + daily buckets, long style
    var bucketstart = new Date(viewstart.getTime());
    while (bucketstart < viewend)
    {
      if (bucketstart.getDay() == 0)
      {
        timeAxis.append('line')
          .attr('class', 'time')
          .attr('x1', Math.floor(x))
          .attr('y1', 0)
          .attr('x2', Math.floor(x))
          .attr('y2', timescaleheight);
        timeAxis.append('text')
          .attr('class', 'svgheadertext')
          .attr('x', Math.floor(x + scaling*7/2))
          .attr('y', timescaleheight/2-1)
          .text($.datepicker.formatDate("yy-mm-dd", bucketstart));
      }
      else
      {
        timeAxis.append('line')
          .attr('class', 'time')
          .attr('x1', Math.floor(x))
          .attr('y1', timescaleheight/2)
          .attr('x2', Math.floor(x))
          .attr('y2', timescaleheight);
      }
      timeAxis.append('text')
        .attr('class', 'svgheadertext')
        .attr('x', Math.floor(x + scaling/2))
        .attr('y', timescaleheight-3)
        .text($.datepicker.formatDate("dd M", bucketstart));
      x = x + scaling;
      bucketstart.setDate(bucketstart.getDate()+1);
    }
  }
  else if (scaling < 350)
  {
    // Weekly + daily buckets, very long style
    var bucketstart = new Date(viewstart.getTime());
    while (bucketstart < viewend)
    {
      if (bucketstart.getDay() == 0)
      {
        timeAxis.append('line')
          .attr('class', 'time')
          .attr('x1', Math.floor(x))
          .attr('y1', 0)
          .attr('x2', Math.floor(x))
          .attr('y2', timescaleheight);
        timeAxis.append('text')
          .attr('class', 'svgheadertext')
          .attr('x', Math.floor(x + scaling*3.5))
          .attr('y', timescaleheight/2-1)
          .text($.datepicker.formatDate("yy-mm-dd", bucketstart));
      }
      else
      {
        timeAxis.append('line')
          .attr('class', 'time')
          .attr('x1', Math.floor(x))
          .attr('y1', timescaleheight/2)
          .attr('x2', Math.floor(x))
          .attr('y2', timescaleheight);
      }
      timeAxis.append('text')
        .attr('class', 'svgheadertext')
        .attr('x', Math.floor(x + scaling/2))
        .attr('y', timescaleheight-3)
        .text($.datepicker.formatDate("D dd M", bucketstart));
      x = x + scaling;
      bucketstart.setDate(bucketstart.getDate()+1);
    }
  }
  else
  {
    // Daily + hourly buckets
    var bucketstart = new Date(viewstart.getTime());
    while (bucketstart < viewend)
    {
      if (bucketstart.getHours() == 0)
      {
        timeAxis.append('line')
          .attr('class', 'time')
          .attr('x1', Math.floor(x))
          .attr('y1', 0)
          .attr('x2', Math.floor(x))
          .attr('y2', timescaleheight);
        timeAxis.append('text')
          .attr('class', 'svgheadertext')
          .attr('x', Math.floor(x + scaling/2))
          .attr('y', timescaleheight/2-1)
          .text($.datepicker.formatDate("D yy-mm-dd", bucketstart));
      }
      else
      {
        timeAxis.append('line')
          .attr('class', 'time')
          .attr('x1', Math.floor(x))
          .attr('y1', timescaleheight/2)
          .attr('x2', Math.floor(x))
          .attr('y2', timescaleheight);
      }
      timeAxis.append('text')
        .attr('class', 'svgheadertext')
        .attr('x', Math.floor(x + scaling/48))
        .attr('y', timescaleheight-3)
        .text(bucketstart.getHours());
      x = x + scaling/24;
      bucketstart.setTime(bucketstart.getTime() + 3600000);
    }
  }
  /*
  $("#jqgh_grid_operationplans")
     .html(result.join(''))
     .unbind('mousedown')
     .bind('mousedown', function(event) {
        gantt.startmousemove = event.pageX;
        $(window).bind('mouseup', function(event) {
          $(window).unbind('mousemove');
          $(window).unbind('mouseup');
          event.stopPropagation();
          })
        $(window).bind('mousemove', function(event) {
          var delta = event.pageX - gantt.startmousemove;
          if (Math.abs(delta) > 3)
          {
            gantt.zoom(1, delta > 0 ? -86400000 : 86400000);
            gantt.startmousemove = event.pageX;
          }
          event.stopPropagation();
        });
        event.stopPropagation();
       });
       */
}


// Encode a string in UTF-8: unescape(encodeURIComponent(s))
