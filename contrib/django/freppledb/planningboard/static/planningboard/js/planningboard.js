
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


function onmessage(ev)
{
  // Debugging message
  console.log(ev.data);

  // Dispatch the message to a handler function
  jsondoc = jQuery.parseJSON(ev.data);
  type = jsondoc.category;
  if (type == "name")
    displayList(jsondoc);
  else if (type=="plan")
    displayPlan(jsondoc);
  else if (type == "chat")
    displayChat(jsondoc);
}


function demandAction (cellvalue, options, row)
{
  var esc = row['name'].replace("'", "\\'");
  return '<span onclick="send(\'/solve/unplan/' + esc + '\')" class="fa fa-stop spacing"></span>' +
    '<span onclick="send(\'/solve/demand/backward/' + esc + '\')" class="fa fa-fast-backward spacing"></span>' +
    '<span onclick="send(\'/solve/demand/forward/' + esc + '\')" class="fa fa-fast-forward spacing"></span>';
}


function displayList(jsondoc)
{
  // Demand tab
  var w = $("#demand").width() - 16;
  $("#demandlist").jqGrid({
    datatype: "local",
    height: 250,
    width: w,
    colModel:[
      {name:'action', width:55, formatter:demandAction, sortable:false, search:false, fixed:true},
      {name:'name', index:'name', key:true, width:90, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'item', index:'item', width:100, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'customer', index:'customer', width:80, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'priority', index:'priority', formatter:'integer', align:'center', width:80, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'quantity', index:'quantity', formatter:'number', align:'right', width:80, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'due', index:'due', width:70, formatter:'date', formatoptions: {srcformat: "Y-m-d\\TH:i:s"}, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'planned quantity', index:'pquantity', width:150, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'planned delivery', index:'pdate', width:150, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}}
    ],
    multiselect: true
  });
  $(jsondoc.demands).each(function() {
    $("#demandlist").jqGrid('addRowData', this.name, {
      'name': this.name,
      'quantity': this.quantity,
      'due': this.due,
      'priority': this.priority,
      'item': this.item,
      'customer': this.customer
      });
    });
  $("#demandlist").jqGrid('filterToolbar',{searchOperators : true});

  // Resource list tab
  $("#resourcelist").jqGrid({
    datatype: "local",
    height: 250,
    width: w,
    colModel:[
      {name:'name', index:'name', key:true, width:90, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'location', index:'location', width:100, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}}
    ],
    multiselect: true
  });
  $(jsondoc.resources).each(function() {
    $("#resourcelist").jqGrid('addRowData', this.name, {
      'name': this.name,
      'location': this.location
      });
    });
  $("#resourcelist").jqGrid('filterToolbar',{searchOperators : true});

  // Buffer list tab
  $("#bufferlist").jqGrid({
    datatype: "local",
    height: 250,
    width: w,
    colModel:[
      {name:'name', index:'name', key: true, width:90, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'item', index:'item', width:100, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'location', index:'location', width:100, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}}
    ],
    multiselect: true
  });
  $(jsondoc.buffers).each(function() {
    $("#bufferlist").jqGrid('addRowData', this.name, {
      'name': this.name,
      'item': this.item,
      'location': this.location
      });
    });
  $("#bufferlist").jqGrid('filterToolbar',{searchOperators : true});

  // Operation list tab
  $("#operationlist").jqGrid({
    datatype: "local",
    height: 250,
    width: w,
    colModel:[
      {name:'name', index:'name', key: true, width:90, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}},
      {name:'location', index:'location', width:100, searchoptions:{sopt:['eq','ne','le','lt','gt','ge']}}
    ],
    multiselect: true
  });
  $(jsondoc.operations).each(function() {
    $("#operationlist").jqGrid('addRowData', this.name, {
      'name': this.name,
      'location': this.location
      });
    });
  $("#operationlist").jqGrid('filterToolbar',{searchOperators : true});

  // Chat history
  displayChat();
}


function displayPlan(jsondoc)
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

  // Display the objects
  $(jsondoc.operations).each(displayOperation);
  $(jsondoc.resources).each(displayResource);
  $(jsondoc.buffers).each(displayBuffer);
  $(jsondoc.demands).each(displayDemand);
}


function dragmove(d)
{
  if (d3.event.dx < 0)
    d3.select(this).attr("x", d3.select(this).attr("x") - 1);
  else if (d3.event.dx > 0)
    d3.select(this).attr("x", d3.select(this).attr("x") + 1);
}


function displayOperation()
{
  // Look up the row to display the information at
  var res = this.name;
  var thisrow = ganttRows['operation/' + res];
  if (thisrow === undefined)
    return; // Operation not to be shown at all

  // Preprocess JSON data
  var data = [];
  var layer = [];
  $(this.operationplans).each(function() {
    var row = 0;
    var strt = new Date(Date.parse(this.start));
    var nd = new Date(Date.parse(this.end))
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
      this.quantity,
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
      .attr("transform", "translate(0," + (thisrow.index*rowheight + timescaleheight) + ")");
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


function displayResource()
{
  // Look up the row to display the information at
  var res = this.name;
  var thisrow = ganttRows['resource/' + res];
  if (thisrow === undefined)
    return; // Resource not to be shown at all

  // Parse JSON data
  var data = [];
  var layer = [];
  $(this.loadplans).each(function() {
    if (this.quantity > 0) {
      var row = 0;
      var strt = new Date(Date.parse(this.operationplan.start));
      var nd = new Date(Date.parse(this.operationplan.end));
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
        this.operationplan.operation,
        strt,
        nd,
        this.quantity,
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
      .attr("transform", "translate(0," + (thisrow.index*rowheight + timescaleheight) + ")");
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


function displayBuffer()
{
  // Look up the row to display the information at
  var res = this.name;
  var thisrow = ganttRows['buffer/' + res];
  if (thisrow === undefined)
    return; // Buffer not to be shown at all

  // Parse JSON data
  var data = [];
  var min_oh = 0;
  var max_oh = 0;
  $(this.flowplans).each(function() {
    var oh = this.onhand;
    data.push([
      new Date(Date.parse(this.date)),
      this.quantity,
      oh,
      this.minimum,
      this.maximum
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
      .attr("transform", "translate(0," + (thisrow.index*rowheight + timescaleheight) + ")");
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


function displayDemand()
{
  // Update the demand grid
  $.jgrid.formatter.date.reformatAfterEdit = true;
  var dmd = this.name;
  $("#demandlist").jqGrid('setRowData', dmd, {
    'name': dmd,
    'quantity': this.quantity,
    'due': this.due,
    'priority': this.priority,
    'item': this.item.name,
    'customer': this.customer.name,
    'planned quantity': 666
    });

  // Look up the row to display the information at
  var thisrow = ganttRows['demand/' + dmd];
  if (thisrow === undefined)
    return; // Demand not to be shown at all

  // Parse JSON data
  var data = [];
  var layer = [];
  $(this.operationplans).each(function() {
    if (this.quantity > 0) {
      var row = 0;
      var strt = new Date(Date.parse(this.start));
      var nd = new Date(Date.parse(this.end));
      for (; row < layer.length; ++row)
      {
        if (nd <= layer[row])
        {
           layer[row] = strt;
           break;
        }
      };
      if (row >= layer.length)
        layer.push(strt);
      data.push([
        this.operation,
        strt,
        nd,
        this.quantity,
        row
        ]);
      }
    });

  // Find existing svg row or create a new one
  if (ganttRows['demand/' + dmd].svg !== null)
  {
    var mysvg = ganttRows['demand/' + dmd].svg;
    mysvg.selectAll("*").remove();
  }
  else
  {
    var mysvg = svg.append("g")
      .attr("transform", "translate(0," + (thisrow.index*rowheight + timescaleheight) + ")");
    ganttRows['demand/' + dmd].svg = mysvg;
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
    .text(dmd)
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


function actionSelectedDemand(action1, action2)
{
  var grid = $("#demandlist");
  var selected = grid.jqGrid('getGridParam','selarrrow');
  if (selected.length == grid.jqGrid('getGridParam', 'reccount'))
    // All rows selected
    send(action1);
  else
    // Subset of rows selected
    for (i in selected)
      send(action2 + selected[i]);
}


function addSelected(entity)
{
   var selected = $("#" + entity + "list").jqGrid('getGridParam','selarrrow');
   for (i in selected)
   {
     var key = entity + "/" + selected[i];
     send("/register/" + key);
     if (!(key in ganttRows))
     {
       // Ask the plan for new entities
       ganttRows[key] = {"index": numRows++, "svg": null};
       send("/plan/" + key);
     }
   }

   // Save the preferences on the server
   var r = [];
   for (var k in ganttRows)
     r[ganttRows[k]["index"]] = k;
   $.ajax({
     url: '/settings/',
     type: 'POST',
     contentType: 'application/json; charset=utf-8',
     data: JSON.stringify({"freppledb.planningboard": {"rows": r}}),
     error: function (result, stat, errorThrown) {
       $('#popup').html(result.responseText)
         .dialog({
           title: gettext("Error saving report settings"),
           autoOpen: true,
           resizable: false,
           width: 'auto',
           height: 'auto'
         });
       }
     });
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
}


function sendChat()
{
  var chatmsg = $("#chatmsg");
  if (chatmsg.val() == "") return;
  send('/chat/' + chatmsg.val());
  chatmsg.val("");
}


function displayChat()
{
  var chatdiv = $("#chathistory");
  $(jsondoc.messages).each(function() {
    chatdiv.append($('<tr>')
      .append($('<td>').text(this.date))
      .append($('<td>').text(this.name))
      .append($('<td>').text(this.value))
      );
  });
  chatdiv = chatdiv.parent();
  chatdiv.scrollTop(chatdiv.prop('scrollHeight'));
}

