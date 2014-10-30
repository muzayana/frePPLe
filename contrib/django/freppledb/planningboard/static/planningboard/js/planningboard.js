
var socket = null;
var curState = 'closed';    // Possible states: closed, connecting, open, disconnecting
var rowindex = 0;

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
           ganttRows = [];
           $("#entities option:selected").each(function() {
             send("/plan/" + this.value);
             send("/register/" + this.value);
             ganttRows.push(this.value);
           });
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


function displayList(xmldoc)
{
  var el = $("#demandlist");
  el.html("");
  $(xmldoc).find('demand').each(function() {
    el.append('&nbsp;&nbsp;' + $(this).attr('name') +
      '&nbsp;&nbsp;<span onclick="send(\'/solve/unplan/' + $(this).attr('name') + '\')" title="Unplan the demand" class="fa fa-step-backward"></span>' +
      '&nbsp;&nbsp;<span onclick="send(\'/solve/demand/' + $(this).attr('name') + '\')" title="Plan the demand" class="fa fa-step-forward"></span><br/>'
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
  width = $("#ganttdiv").width() - 24;
  height = ganttRows.length * rowheight;
  $('#ganttdiv').resizable('option', 'maxHeight', height + 10);
  svg = d3.select("#gantt")
    .attr("width", width)
    .attr("height", height);
  svg.selectAll("*").remove();
  rowindex = 0;
  // Create a scale for the x-axis and the y-axis
  x = d3.time.scale()
    .domain([horizonstart, horizonend])
    .range([0, width]);

  // Dragging function
  drag = d3.behavior.drag()
    .origin(function(d) { return d; })
    .on("drag", dragmove);

  // Display the operations
  $(xmldoc).find('operations').children().each(function() {
    displayOperation(this, rowindex++);
  });

  // Display the resources
  $(xmldoc).find('resources').children().each(function() {
    displayResource(this, rowindex++);
  });

  // Display the buffers
  $(xmldoc).find('buffers').children().each(function() {
    displayBuffer(this, rowindex++);
    });
}


function dragmove(d)
{
  console.log(d + "  " + d3.event.x + "   " + d3.event.y)
  //d3.select(this)
  //  .attr("x", d3.event.x);
}


function displayOperation(xml, indx)
{
  // Parse XML data
  var res = $(xml).attr('name');
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

  // Draw Gantt chart
  var mysvg = svg.append("g")
    .attr("transform", "translate(0," + (indx*rowheight) + ")");
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
  // Parse XML data
  var res = $(xml).attr('name');
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

  // Draw Gantt chart
  var mysvg = svg.append("g").attr("transform", "translate(0," + (indx*rowheight) + ")");
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
  // Parse XML data
  var res = $(xml).attr('name');
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

  // Draw Gantt chart
  var mysvg = svg.append("g").attr("transform", "translate(0," + (indx*rowheight) + ")");
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
  var line = d3.svg.line()
    .x(function(d) { return x(d[0]); })
    .y(function(d) { return y(d[2]); });
  mysvg.append("g")
    .attr("transform", "translate(250,0)")
    .append("path")
    .attr('class', 'graphline')
    .attr("stroke","#8BBA00")
    .attr("d", line(data));
}


// Encode a string in UTF-8: unescape(encodeURIComponent(s))
