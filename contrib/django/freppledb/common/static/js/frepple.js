
// Django sets this variable in the admin/base.html template.
window.__admin_media_prefix__ = "/static/admin/";



// Adjust the breadcrumbs such that it fits on a single line.
// This function is called when the window is resized.
function breadcrumbs_reflow()
{
  var crumbs = $("#breadcrumbs");
  var height_one_line = Math.ceil($("#cockpitcrumb").height()) + 16;

  // Show all elements previously hidden
  crumbs.children("li:hidden").show();
  // Hide the first crumbs till it all fits on a single line.
  var first = true;
  crumbs.children("li").each(function() {
    if (crumbs.height() > height_one_line && !first) $(this).hide();
    first = false;
  });
}


// A function to escape all special characters in a name.
// We escape all special characters in the EXACT same way as the django admin does.
function admin_escape(n)
{
  return n.replace(/_/g,'_5F').replace(/&amp;/g,'_26').replace(/&lt;/g,'_3C')
  .replace(/&gt;/g,'_3E').replace(/&#39;/g,"'").replace(/&quot;/g,'_22')
  .replace(/:/g,'_3A').replace(/\//g,'_2F').replace(/#/g,'_23').replace(/\?/g,'_3F')
  .replace(/;/g,'_3B').replace(/@/g,'_40').replace(/&/g,'_26').replace(/=/g,'_3D')
  .replace(/\+/g,'_2B').replace(/\$/g,'_24').replace(/,/g,'_2C').replace(/"/g,'_22')
  .replace(/</g,'_3C').replace(/>/g,'_3E').replace(/%/g,'_25').replace(/\\/g,'_5C');
}


//----------------------------------------------------------------------------
// A class to handle changes to a grid.
//----------------------------------------------------------------------------
var upload = {
  warnUnsavedChanges: function()
  {
    $(window).off('beforeunload', upload.warnUnsavedChanges);
    return gettext("There are unsaved changes on this page.");
  },

  undo : function ()
  {
    if ($('#undo').hasClass("btn-primary")) return;
    $("#grid").trigger("reloadGrid");
    $("#grid").closest(".ui-jqgrid-bdiv").scrollTop(0);
    $('#save, #undo').addClass("btn-primary").removeClass("btn-danger").prop('disabled', true);
    $('#actions1').prop('disabled', true);

    $('#filter').prop('disabled', false);
    $(window).off('beforeunload', upload.warnUnsavedChanges);
  },

  select : function ()
  {
    $('#filter').prop('disabled', true);
    $.jgrid.hideModal("#searchmodfbox_grid");
    $('#save, #undo').removeClass("btn-primary").addClass("btn-danger").prop('disabled', false);
    $(window).off('beforeunload', upload.warnUnsavedChanges);
    $(window).on('beforeunload', upload.warnUnsavedChanges);
  },

  save : function()
  {
    if ($('#save').hasClass("btn-primary")) return;

    // Pick up all changed cells. If a function "getData" is defined on the
    // page we use that, otherwise we use the standard functionality of jqgrid.
    $("#grid").saveCell(editrow, editcol);
    if (typeof getDirtyData == 'function')
      var rows = getDirtyData();
    else
      var rows = $("#grid").getChangedCells('dirty');
    if (rows != null && rows.length > 0)
      // Send the update to the server
      $.ajax({
          url: location.pathname,
          data: JSON.stringify(rows),
          type: "POST",
          contentType: "application/json",
          success: function () {
            upload.undo();
            },
          error: function (result, stat, errorThrown) {
              $('#timebuckets').modal('hide');
            $.jgrid.hideModal("#searchmodfbox_grid");
              $('#popup').html('<div class="modal-dialog">'+
                      '<div class="modal-content">'+
                        '<div class="modal-header">'+
                          '<button type="button" class="close" data-dismiss="modal"><span aria-hidden="true" class="fa fa-times"></span></button>'+
                          '<h4 class="modal-title alert alert-danger">'+ gettext("Error saving data")+'</h4>'+
                        '</div>'+
                        '<div class="modal-body">'+
                          '<p>'+result.responseText+'</p>'+
                        '</div>'+
                        '<div class="modal-footer">'+
                          '<input type="submit" id="cancelbutton" role="button" class="btn btn-primary pull-right" data-dismiss="modal" value="'+gettext('Close')+'">'+
                        '</div>'+
                      '</div>'+
                  '</div>' )
                  .modal('show');
            }
        });
  },

  validateSort: function(event)
  {
    if ($(this).attr('id') == 'grid_cb') return;
    if ($('#save').hasClass("btn-primary"))
      jQuery("#grid").jqGrid('resetSelection');
    else
    {
      $('#timebuckets').modal('hide');
      $.jgrid.hideModal("#searchmodfbox_grid");
      $('#popup').html('<div class="modal-dialog">'+
          '<div class="modal-content">'+
          '<div class="modal-header">'+
            '<h4 class="modal-title alert-warning">'+ gettext("Save or cancel your changes first") +'</h4>'+
          '</div>'+
          '<div class="modal-body">'+
            '<p>'+""+'</p>'+
          '</div>'+
          '<div class="modal-footer">'+
            '<input type="submit" id="savebutton" role="button" class="btn btn-primary pull-right" value="'+gettext('Save')+'">'+
            '<input type="submit" id="cancelbutton" role="button" class="btn btn-primary pull-right" value="'+gettext('Cancel')+'">'+
          '</div>'+
        '</div>'+
      '</div>' )
      .modal('show');
      $('#savebutton').on('click', function() {
                upload.save();
        $('#popup').modal('hide');
      });
      $('#cancelbutton').on('click', function() {
                upload.undo();
        $('#popup').modal('hide');
        });
      event.stopPropagation();
    }
  }
}

//----------------------------------------------------------------------------
// Custom formatter functions for the grid cells.
//----------------------------------------------------------------------------

function opendetail(event) {
  var database = $('#database').prop('name');
  database = (database===undefined || database==='default') ? '' : '/' + database;
  var curlink = $(event.target).parent().attr('href');
  var objectid = $(event.target).parent().parent().text();
  objectid = admin_escape(objectid);

  event.preventDefault();
  event.stopImmediatePropagation();
  window.location.href = database + curlink.replace('key', objectid);
}

jQuery.extend($.fn.fmatter, {
  percentage : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue === '' || cellvalue === null) return '';
    return cellvalue + "%";
  },
  duration : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue === '' || cellvalue === null) return '';
    var d = cellvalue.split(" ");
    if (d.length == 1)
    {
      var t = cellvalue.split(":");
      var days = 0;
    }
    else
    {
      var t = d[1].split(":");
      var days = (d[0]!='' ? parseFloat(d[0]) : 0);
    }
    switch (t.length)
    {
      case 0: // Days only
        var seconds = days * 86400;
        break;
      case 1: // Days, seconds
        var seconds = days * 86400 + (t[0]!='' ? parseFloat(t[0]) : 0);
        break;
      case 2: // Days, hours and seconds
        var seconds = days * 86400 + (t[0]!='' ? parseFloat(t[0]) : 0) * 60 + (t[1]!='' ? parseFloat(t[1]) : 0);
        break;
      default:
        // Days, hours, minutes, seconds
        var seconds = days * 86400 + (t[0]!='' ? parseFloat(t[0]) : 0) * 3600 + (t[1]!='' ? parseFloat(t[1]) : 0) * 60 + (t[2]!='' ? parseFloat(t[2]) : 0);
    }
    var days   = Math.floor(seconds / 86400);
    var hours   = Math.floor((seconds - (days * 86400)) / 3600);
    var minutes = Math.floor((seconds - (days * 86400) - (hours * 3600)) / 60);
    var seconds = seconds - (days * 86400) - (hours * 3600) - (minutes * 60);
    if (days > 0)
      return days + ((hours < 10) ? " 0" : " ") + hours + ((minutes < 10) ? ":0" : ":") + minutes + ((seconds < 10) ? ":0" : ":") + seconds;
    if (hours > 0)
      return hours + ((minutes < 10) ? ":0" : ":") + minutes + ((seconds < 10) ? ":0" : ":") + seconds;
    if (minutes > 0)
      return minutes + ((seconds < 10) ? ":0" : ":") + seconds;
    return seconds;
  },

  detail : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue === '' || cellvalue === null) return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<a href='/detail/" + options.colModel.role + "/key/' onclick='opendetail(event)'><span class='leftpadding fa fa-caret-right' role='" + options.colModel.role + "'></span></a>";
  },

  demanddetail : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue === '') return '';
    if (options['colModel']['popup']) return cellvalue;
    var result = '';
    var dmds = cellvalue.split(", ");
    for (var i in dmds)
    {
      var detail = dmds[i].split(" : ");
      if (result != '') result += ', ';
      result += detail[0] + " : <span>" + detail[1] + "<span class='context fa fa-caret-right' role='demand'></span></span>"
    }
    return result;
  },

  graph : function (cellvalue, options, rowdata) {
    return '<div class="graph" style="height:80px"></div>';
  }
});
jQuery.extend($.fn.fmatter.percentage, {
    unformat : function(cellvalue, options, cell) {
      return cellvalue;
      }
});


//
// Functions related to jqgrid
//

var grid = {

   // Popup row selection.
   // The popup window present a list of objects. The user clicks on a row to
   // select it and a "select" button appears. When this button is clicked the
   // popup is closed and the selected id is passed to the calling page.
   selected: undefined,

   // Function used to summarize by returning the last value
   summary_last: function(val, name, record)
   {
     return record[name];
   },

   // Function used to summarize by returning the first value
   summary_first: function(val, name, record)
   {
     return val || record[name];
   },

   setSelectedRow: function(id)
   {
     if (grid.selected != undefined)
       $(this).jqGrid('setCell', grid.selected, 'select', null);
     grid.selected = id;
     $(this).jqGrid('setCell', id, 'select', '<input type="checkbox" onClick="opener.dismissRelatedLookupPopup(window, grid.selected);" class="btn btn-primary" style="width: 18px; height: 18px;" data-toggle="tooltip" title="'+gettext('Click to select record')+'"></input>');
   },

   runAction: function(next_action) {
    if ($("#actions").val() != "no_action")
       actions[$("#actions").val()]();
   },

   setStatus : function(newstatus)
   {
    var sel = jQuery("#grid").jqGrid('getGridParam','selarrrow');
    for ( i in sel ) {
      jQuery("#grid").jqGrid("setCell", sel[i], "status", newstatus, "dirty-cell");
      jQuery("#grid").jqGrid("setRowData", sel[i], false, "edited");
    };
    $("#actions1").html($("#actionsul").children().first().text() + '  <span class="caret"></span>');
    $('#save').removeClass("btn-primary").addClass("btn-danger").prop("disabled",false);
    $('#undo').removeClass("btn-primary").addClass("btn-danger").prop("disabled",false);
   },

  // Renders the cross list in a pivot grid
  pivotcolumns : function  (cellvalue, options, rowdata)
  {
    var result = '';
    for (i in cross_idx)
    {
      if (result != '') result += '<br/>';
      if (cross[cross_idx[i]]['editable'])
        result += '<span class="editablepivotcol">' + cross[cross_idx[i]]['name'] + '</span>';
      else
        result += cross[cross_idx[i]]['name'];
    }
    return result;
  },

  // Render the customization popup window
  showCustomize: function (pivot)
  {
    var colModel = $("#grid")[0].p.colModel;
    var maxfrozen = 0;
    var skipped = 0;
    var graph = false;

    var row0 = ""+
      '<div class="row">' +
      '<div class="col-xs-6">' +
        '<div class="panel panel-default"><div class="panel-heading">'+ gettext("Selected options") + '</div>' +
          '<div class="panel-body">' +
            '<ul class="list-group" id="Rows" style="height: 160px; overflow-y: scroll;">placeholder0</ul>' +
          '</div>' +
        '</div>'+
      '</div>' +
      '<div class="col-xs-6">' +
        '<div class="panel panel-default"><div class="panel-heading">' + gettext("Available options") + '</div>' +
          '<div class="panel-body">' +
            '<ul class="list-group" id="DroppointRows" style="height: 160px; overflow-y: scroll;">placeholder1</ul>' +
          '</div>' +
        '</div>' +
      '</div>' +
    '</div>';

    row1= "";
    row2= "";

    var val0s = ""; //selected columns
    var val0a = ""; //available columns
    var val1s = ""; //selected columns
    var val1a = ""; //available columns

    for (var i in colModel)
    {
      if (colModel[i].name == 'graph')
        graph = true;
      else if (colModel[i].name != "rn" && colModel[i].name != "cb" && colModel[i].counter != null && colModel[i].label != '' && !('alwayshidden' in colModel[i]))
      {
        if (colModel[i].frozen) maxfrozen = parseInt(i,10) + 1 - skipped;
        if (!colModel[i].hidden) {
          val0s += '<li id="' + (i) + '"  class="list-group-item">' + colModel[i].label + '</li>';
        } else {
          val0a += '<li id="' + (i) + '"  class="list-group-item">' + colModel[i].label + '</li>';
        }
      }
      else
        skipped++;
    }

    if (pivot)
    {
      // Add list of crosses
      var row1 = ''+
      '<div class="row">' +
        '<div class="col-xs-6">' +
          '<div class="panel panel-default">' +
            '<div class="panel-heading">' +
              gettext('Selected Cross') +
            '</div>' +
            '<div class="panel-body">' +
              '<ul class="list-group" id="Crosses" style="height: 160px; overflow-y: scroll;">placeholder0</ul>' +
            '</div>' +
          '</div>' +
        '</div>' +
        '<div class="col-xs-6">' +
          '<div class="panel panel-default">' +
            '<div class="panel-heading">' +
              gettext('Available Cross') +
            '</div>' +
            '<div class="panel-body">' +
              '<ul class="list-group" id="DroppointCrosses" style="height: 160px; overflow-y: scroll;">placeholder1</ul>' +
            '</div>' +
          '</div>' +
        '</div>' +
      '</div>';
      for (var j in cross_idx)
      {
        val1s += '<li class="list-group-item" id="' + (100+parseInt(cross_idx[j],10)) + '">' + cross[cross_idx[j]]['name'] + '</li>';
      }
      for (var j in cross)
      {
        if (cross_idx.indexOf(parseInt(j,10)) > -1) continue;
        val1a += '<li class="list-group-item" id="' + (100 + parseInt(j,10) ) + '">' + cross[j]['name'] + '</li>';
      }
    }
    else
    {
      // Add selection of number of frozen columns
      row2 = '<div class="row"><div class="col-xs-12">' +
        gettext("Frozen columns") +
        "<input type='number' id='frozen' min='0' max='4' step='1'>" +
       '</div></div>';
    }

    row0 = row0.replace('placeholder0',val0s);
    row0 = row0.replace('placeholder1',val0a);
    if (pivot) {
      row1 = row1.replace('placeholder0',val1s);
      row1 = row1.replace('placeholder1',val1a);
    }

    $('#popup').html(''+
      '<div class="modal-dialog">'+
        '<div class="modal-content">'+
          '<div class="modal-header">'+
            '<button type="button" class="close" data-dismiss="modal" aria-label=' + gettext("Close") + '>' +
              '<span aria-hidden="true">&times;</span>' +
            '</button>'+
            '<h4 class="modal-title">'+gettext("Customize")+'</h4>'+
          '</div>'+
          '<div class="modal-body">'+
            row0 +
            row1 +
            row2 +
          '</div>' +
          '<div class="modal-footer">'+
            '<input type="submit" id="okCustbutton" role="button" class="btn btn-danger pull-left" value="'+gettext("OK")+'">'+
            '<input type="submit" id="cancelCustbutton" role="button" class="btn btn-primary pull-right" data-dismiss="modal" value="'+gettext('Cancel')+'">'+
            '<input type="submit" id="resetCustbutton" role="button" class="btn btn-primary pull-right" value="'+gettext('Reset')+'">'+
          '</div>'+
        '</div>'+
      '</div>' )
    .modal('show');

    var Rows = document.getElementById("Rows");
    var DroppointRows = document.getElementById("DroppointRows");
    Sortable.create(Rows, {
      group: {
        name: 'Rows',
        put: ['DroppointRows']
      },
      animation: 100
    });
    Sortable.create(DroppointRows, {
      group: {
        name: 'DroppointRows',
        put: ['Rows']
      },
      animation: 100
    });

    if (pivot) {
      var Crosses = document.getElementById("Crosses");
      var DroppointCrosses = document.getElementById("DroppointCrosses");
      Sortable.create(Crosses, {
        group: {
          name: 'Crosses',
          put: ['DroppointCrosses']
        },
        animation: 100
      });
      Sortable.create(DroppointCrosses, {
        group: {
          name: 'DroppointCrosses',
          put: ['Crosses']
        },
        animation: 100
      });
    }

    $('#resetCustbutton').on('click', function() {
      var result = {};
      result[reportkey] = null;
      if (typeof url_prefix != 'undefined')
        var url = url_prefix + '/settings/';
      else
        var url = '/settings/';
      $.ajax({
       url: url,
       type: 'POST',
       contentType: 'application/json; charset=utf-8',
       data: JSON.stringify(result),
       success: function() {window.location.href = window.location.href;},
       error: function (result, stat, errorThrown) {
         $('#popup').html('<div class="modal-dialog" style="width: auto">'+
             '<div class="modal-content">'+
             '<div class="modal-header">'+
               '<button type="button" class="close" data-dismiss="modal"><span aria-hidden="true" class="fa fa-times"></span></button>'+
               '<h4 class="modal-title">{% trans "Error retrieving data" %}</h4>'+
             '</div>'+
             '<div class="modal-body">'+
               '<p>'+result.responseText + "  " + stat + errorThrown+'</p>'+
             '</div>'+
             '<div class="modal-footer">'+
             '</div>'+
           '</div>'+
           '</div>' ).modal('show');
         }
       });
     });

    $('#okCustbutton').on('click', function() {
      var colModel = $("#grid")[0].p.colModel;
      var perm = [];
      var hiddenrows = [];
      if (colModel[0].name == "cb") perm.push(0);
      cross_idx = [];
      if (!graph)
        $("#grid").jqGrid('destroyFrozenColumns');

      $('#Rows li').each(function() {
        val = parseInt(this.id,10);
        if (val < 100)
        {
            $("#grid").jqGrid("showCol", colModel[val].name);
            perm.push(val);
         }
      });

      $('#DroppointRows li').each(function() {
        val = parseInt(this.id,10);
        if (val < 100)
        {
          hiddenrows.push(val);
          if (pivot)
            $("#grid").jqGrid('setColProp', colModel[val].name, {frozen:false});
          $("#grid").jqGrid("hideCol", colModel[val].name);
         }
      });

      $('#Crosses li').each(function() {
        val = parseInt(this.id,10);
        if (val >= 100)
        {
          cross_idx.push(val-100);
         }
      });

      var numfrozen = 0;
      if (pivot)
      {
        var firstnonfrozen = 0;
        for (var i in colModel)
          if ("counter" in colModel[i])
            numfrozen = i+1;
          else
            perm.push(parseInt(i,10));
      }
      else
        numfrozen = parseInt($("#frozen").val())
      for (var i in hiddenrows)
        perm.push(hiddenrows[i]);
      $("#grid").jqGrid("remapColumns", perm, true);
      var skipped = 0;
      for (var i in colModel)
        if (colModel[i].name != "rn" && colModel[i].name != "cb" && colModel[i].counter != null)
          $("#grid").jqGrid('setColProp', colModel[i].name, {frozen:i-skipped<numfrozen});
        else
          skipped++;
      if (!graph)
        $("#grid").jqGrid('setFrozenColumns');
      $("#grid").trigger('reloadGrid');
      grid.saveColumnConfiguration();
      $('#popup').modal("hide");
    });
  },

  // Save the customized column configuration
  saveColumnConfiguration : function(pgButton, indx)
  {
    // This function can be called with different arguments:
    //   - no arguments, when called from our code
    //   - paging button string, when called from jqgrid paging event
    //   - number argument, when called from jqgrid resizeStop event
    var colArray = new Array();
    var colModel = $("#grid")[0].p.colModel;
    var maxfrozen = 0;
    var pivot = false;
    var skipped = 0;
    var page = $('#grid').getGridParam('page');
    if (typeof pgButton === 'string')
    {
      // JQgrid paging gives only the current page
      if (pgButton.indexOf("next") >= 0)
        ++page;
      else if (pgButton.indexOf("prev") >= 0)
        --page;
      else if (pgButton.indexOf("last") >= 0)
        page = $("#grid").getGridParam('lastpage');
      else if (pgButton.indexOf("first") >= 0)
        page = 1;
      else if (pgButton.indexOf("user") >= 0)
        page = $('input.ui-pg-input').val();
    }
    else if (typeof indx != 'undefined' && colModel[indx].name == "operationplans")
      // We're resizing a Gantt chart column. Not too clean to trigger the redraw here, but so be it...
      gantt.redraw();
    for (var i in colModel)
    {
      if (colModel[i].name != "rn" && colModel[i].name != "cb" && "counter" in colModel[i] && !('alwayshidden' in colModel[i]))
      {
        colArray.push([colModel[i].counter, colModel[i].hidden, colModel[i].width]);
        if (colModel[i].frozen) maxfrozen = parseInt(i) + 1 - skipped;
      }
      else if (colModel[i].name == 'columns' || colModel[i].name == 'graph')
        pivot = true;
      else
        skipped++;
    }
    var result = {};
    var filter = $('#grid').getGridParam("postData").filters;
    if (typeof filter !== 'undefined' && filter.rules != [])
      result[reportkey] = {
        "rows": colArray,
        "page": page,
        "filter": filter
        };
    else
      result[reportkey] = {
        "rows": colArray,
        "page": page,
        };
    var sidx = $('#grid').getGridParam('sortname');
    if (sidx !== '')
    {
      // Report is sorted
      result[reportkey]['sidx'] = sidx;
      result[reportkey]['sord'] = $('#grid').getGridParam('sortorder');
    }
    if (pivot)
      result[reportkey]['crosses'] = cross_idx;
    else
      result[reportkey]['frozen'] = maxfrozen;
    if(typeof extraPreference == 'function')
    {
      var extra = extraPreference();
      for (var idx in extra)
        result[reportkey][idx] = extra[idx];
    }
    if (typeof url_prefix != 'undefined')
      var url = url_prefix + '/settings/';
    else
      var url = '/settings/';
    $.ajax({
      url: url,
      type: 'POST',
      contentType: 'application/json; charset=utf-8',
      data: JSON.stringify(result),
      error: function (result, stat, errorThrown) {
        $('#popup').html('<div class="modal-dialog" style="width: auto">'+
            '<div class="modal-content">'+
            '<div class="modal-header">'+
              '<button type="button" class="close" data-dismiss="modal"><span aria-hidden="true" class="fa fa-times"></span></button>'+
              '<h4 class="modal-title">{% trans "Error saving report settings" %}</h4>'+
            '</div>'+
            '<div class="modal-body">'+
              '<p>'+result.responseText + "  " + stat + errorThrown+'</p>'+
            '</div>'+
            '<div class="modal-footer">'+
            '</div>'+
          '</div>'+
          '</div>' ).modal('show');
      }
    });
  },

  //This function is called when a cell is just being selected in an editable
  //grid. It is used to either a) select the content of the cell (to make
  //editing it easier) or b) display a date picker it the field is of type
  //date.
  afterEditCell: function (rowid, cellname, value, iRow, iCol)
  {
  var colmodel = $(this).jqGrid('getGridParam', 'colModel')[iCol];
  icons = {
      time: 'fa fa-clock-o',
      date: 'fa fa-calendar',
      up: 'fa fa-chevron-up',
      down: 'fa fa-chevron-down',
      previous: 'fa fa-chevron-left',
      next: 'fa fa-chevron-right',
      today: 'fa fa-bullseye',
      clear: 'fa fa-trash',
      close: 'fa fa-remove'
    };

  if (colmodel.formatter == 'date')
  {
    if (colmodel.formatoptions['srcformat'] == "Y-m-d")
      $("#" + iRow + '_' + cellname).datetimepicker({format: 'YYYY-MM-DD', calendarWeeks: true, icons, locale: document.documentElement.lang});
    else
      $("#" + iRow + '_' + cellname).datetimepicker({format: 'YYYY-MM-DD HH:mm:ss', calendarWeeks: true, icons, locale: document.documentElement.lang});
  }
  else
	$("#" + iRow + '_' + cellname).select();
  },

  showExport: function(only_list)
  {
    $('#timebuckets').modal('hide');
    $.jgrid.hideModal("#searchmodfbox_grid");
    // The argument is true when we show a "list" report.
    // It is false for "table" reports.
    if (only_list)
      $('#popup').html('<div class="modal-dialog">'+
          '<div class="modal-content">'+
            '<div class="modal-header">'+
              '<button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>'+
              '<h4 class="modal-title">'+gettext("Export CSV or Excel file")+'</h4>'+
            '</div>'+
            '<div class="modal-body">'+
        gettext("Export format") + '&nbsp;&nbsp;:&nbsp;&nbsp;<select name="csvformat" id="csvformat">' +
        '<option value="spreadsheetlist" selected="selected">' + gettext("Spreadsheet list") + '</option>' +
              '<option value="csvlist">' + gettext("CSV list") +'</option></select>' +
            '</div>'+
            '<div class="modal-footer">'+
              '<input type="submit" id="exportbutton" role="button" class="btn btn-danger pull-left" value="'+gettext('Export')+'">'+
              '<input type="submit" id="cancelbutton" role="button" class="btn btn-primary pull-right" data-dismiss="modal" value="'+gettext('Cancel')+'">'+
            '</div>'+
          '</div>'+
      '</div>' )
      .modal('show');
    else
      $('#popup').html('<div class="modal-dialog">'+
          '<div class="modal-content">'+
            '<div class="modal-header">'+
              '<h4 class="modal-title">'+gettext("Export CSV or Excel file")+'</h4>'+
            '</div>'+
            '<div class="modal-body">'+
              gettext("Export format") + '&nbsp;&nbsp;:&nbsp;&nbsp;'+
              '<select name="csvformat" id="csvformat">' +
        '<option value="spreadsheettable" selected="selected">' + gettext("Spreadsheet table") + '</option>' +
        '<option value="spreadsheetlist">' + gettext("Spreadsheet list") + '</option>' +
        '<option value="csvtable">' + gettext("CSV table") +'</option>'+
                '<option value="csvlist">' + gettext("CSV list") +'</option>'+
              '</select>' +
            '</div>'+
            '<div class="modal-footer">'+
              '<input type="submit" id="exportbutton" role="button" class="btn btn-danger pull-left" value="'+gettext('Export')+'">'+
              '<input type="submit" id="cancelbutton" role="button" class="btn btn-primary pull-right" data-dismiss="modal" value="'+gettext('Cancel')+'">'+
            '</div>'+
          '</div>'+
      '</div>' )
      .modal('show');
    $('#exportbutton').on('click', function() {
              // Fetch the report data
              var url = (location.href.indexOf("#") != -1 ? location.href.substr(0,location.href.indexOf("#")) : location.href);
              if (location.search.length > 0)
                // URL already has arguments
                url += "&format=" + $('#csvformat').val();
              else if (url.charAt(url.length - 1) == '?')
                // This is the first argument for the URL, but we already have a question mark at the end
                url += "format=" + $('#csvformat').val();
              else
                // This is the first argument for the URL
                url += "?format=" + $('#csvformat').val();
              // Append current filter and sort settings to the URL
              var postdata = $("#grid").jqGrid('getGridParam', 'postData');
              url +=  "&" + jQuery.param(postdata);
              // Open the window
              window.open(url,'_blank');
      $('#popup').modal('hide');
    })
          },


  // Display time bucket selection dialog
  showBucket: function()
  {
    // Show popup
    $('#popup').modal('hide');
    $.jgrid.hideModal("#searchmodfbox_grid");
    icons = {
      time: 'fa fa-clock-o',
      date: 'fa fa-calendar',
      up: 'fa fa-clock-o',
      down: 'fa fa-chevron-down',
      previous: 'fa fa-chevron-left',
      next: 'fa fa-chevron-right',
      today: 'fa fa-bullseye',
      clear: 'fa fa-trash',
      close: 'fa fa-remove'
    };
    $( "#horizonstart" ).datetimepicker({format: 'YYYY-MM-DD', calendarWeeks: true, icons, locale: document.documentElement.lang});
    $( "#horizonend" ).datetimepicker({format: 'YYYY-MM-DD', calendarWeeks: true, icons, locale: document.documentElement.lang});
    $("#horizonstart").on("dp.change", function (selected) {
      $("#horizonend").data("DateTimePicker").minDate(selected.date);
      });
    $( "#okbutton" ).on('click', function() {
            // Compare old and new parameters
            var params = $('#horizonbuckets').val() + '|' +
              $('#horizonstart').val() + '|' +
              $('#horizonend').val() + '|' +
              ($('#horizontype').is(':checked') ? "True" : "False") + '|' +
              $('#horizonlength').val() + '|' +
              $('#horizonunit').val();

            if (params == $('#horizonoriginal').val())
              // No changes to the settings. Close the popup.
              $(this).modal('hide');
            else {
              // Ajax request to update the horizon preferences
              $.ajax({
                  type: 'POST',
                  url: '/horizon/',
                  data: {
                    horizonbuckets: $('#horizonbuckets').val(),
                    horizonstart: $('#horizonstart').val(),
                    horizonend: $('#horizonend').val(),
                    horizontype: ($('#horizontype').is(':checked') ? '1' : '0'),
                    horizonlength: $('#horizonlength').val(),
                    horizonunit: $('#horizonunit').val()
                    },
                  dataType: 'text/html',
                  async: false  // Need to wait for the update to be processed!
                });
            // Reload the report
            window.location.href = window.location.href;
            }});
    $('#timebuckets').modal('show');
         },

  //Display dialog for copying or deleting records
  showDelete : function()
  {
    if ($('#delete_selected').hasClass("disabled")) return;
    var sel = jQuery("#grid").jqGrid('getGridParam','selarrrow');
    if (sel.length == 1)
    {
      // Redirect to a page for deleting a single entity
      location.href = location.pathname + encodeURI(sel[0]) + '/delete/';
    }
    else if (sel.length > 0)
    {
     $('#timebuckets').modal('hide');
     $.jgrid.hideModal("#searchmodfbox_grid");
     $('#popup').html('<div class="modal-dialog">'+
             '<div class="modal-content">'+
               '<div class="modal-header">'+
                 '<button type="button" class="close" data-dismiss="modal"><span aria-hidden="true" class="fa fa-times"></span></button>'+
                 '<h4 class="modal-title">'+gettext('Delete data')+'</h4>'+
               '</div>'+
               '<div class="modal-body">'+
                 '<p>'+interpolate(gettext('You are about to delete %s objects AND ALL RELATED RECORDS!'), [sel.length], false)+'</p>'+
               '</div>'+
               '<div class="modal-footer">'+
                 '<input type="submit" id="delbutton" role="button" class="btn btn-danger pull-left" value="'+gettext('Confirm')+'">'+
                 '<input type="submit" id="cancelbutton" role="button" class="btn btn-primary pull-right" data-dismiss="modal" value="'+gettext('Cancel')+'">'+
               '</div>'+
             '</div>'+
         '</div>' )
         .modal('show');
     $('#delbutton').on('click', function() {
               $.ajax({
                 url: location.pathname,
                 data: JSON.stringify([{'delete': sel}]),
                 type: "POST",
                 contentType: "application/json",
                 success: function () {
               $("#delete_selected").prop("disabled", true).removeClass("bold");
               $("#copy_selected").prop("disabled", true).removeClass("bold");
                   $('.cbox').prop("checked", false);
                   $('#cb_grid.cbox').prop("checked", false);
                   $("#grid").trigger("reloadGrid");
               $('#popup').modal('hide');
                   },
                 error: function (result, stat, errorThrown) {
               $('#popup .modal-body p').html(result.responseText);
               $('#popup .modal-title').addClass("alert alert-danger").html(gettext("Error deleting data"));
               $('#delbutton').prop("disabled", true).hide();
                   }
           })
         })
             }
           },

  showCopy: function()
  {
   if ($('#copy_selected').hasClass("disabled")) return;
   var sel = jQuery("#grid").jqGrid('getGridParam','selarrrow');
   if (sel.length > 0)
   {
     $('#timebuckets').modal('hide');
     $.jgrid.hideModal("#searchmodfbox_grid");
     $('#popup').html('<div class="modal-dialog">'+
             '<div class="modal-content">'+
               '<div class="modal-header">'+
                 '<button type="button" class="close" data-dismiss="modal"><span aria-hidden="true" class="fa fa-times"></span></button>'+
                 '<h4 class="modal-title">'+gettext("Copy data")+'</h4>'+
                 '</div>'+
                 '<div class="modal-body">'+
                   '<p>'+interpolate(gettext('You are about to duplicate %s objects'), [sel.length], false)+'</p>'+
                   '</div>'+
                   '<div class="modal-footer">'+
                     '<input type="submit" id="copybutton" role="button" class="btn btn-danger pull-left" value="'+gettext('Confirm')+'">'+
                     '<input type="submit" id="cancelbutton" role="button" class="btn btn-primary pull-right" data-dismiss="modal" value="'+gettext('Cancel')+'">'+
                   '</div>'+
                 '</div>'+
             '</div>' )
     .modal('show');
     $('#copybutton').on('click', function() {
               $.ajax({
                 url: location.pathname,
                 data: JSON.stringify([{'copy': sel}]),
                 type: "POST",
                 contentType: "application/json",
                 success: function () {
           $("#delete_selected").prop("disabled", true).removeClass("bold");
           $("#copy_selected").prop("disabled", true).removeClass("bold");
                   $('.cbox').prop("checked", false);
                   $('#cb_grid.cbox').prop("checked", false);
                   $("#grid").trigger("reloadGrid");
           $('#popup').modal('hide');
                   },
                 error: function (result, stat, errorThrown) {
           $('#popup .modal-body p').html(result.responseText);
           $('#popup .modal-title').addClass("alert alert-danger").html(gettext("Error copying data"));
           $('#copybutton').prop("disabled", true).hide();
                   }
       })
     })
   }
  },

  // Display filter dialog
  showFilter: function()
  {
    if ($('#filter').hasClass("disabled")) return;
    $('.modal').modal('hide');
    jQuery("#grid").jqGrid('searchGrid', {
      closeOnEscape: true,
      multipleSearch:true,
      multipleGroup:true,
      overlay: 0,
      sopt: ['eq','ne','lt','le','gt','ge','bw','bn','in','ni','ew','en','cn','nc'],
      onSearch : function() {
        grid.saveColumnConfiguration();
        var s = grid.getFilterGroup(jQuery("#fbox_grid").jqFilter('filterData'), true);
        $('#curfilter').html( s ? gettext("Filtered where") + " " + s : "" );
        },
      onReset : function() {
        if (typeof initialfilter !== 'undefined' )
        {
          $("#grid").jqGrid('getGridParam','postData').filters = JSON.stringify(initialfilter);
          $('#curfilter').html(gettext("Filtered where") + " " + grid.getFilterGroup(initialfilter, true) );
        }
        else
          $('#curfilter').html("");
        grid.saveColumnConfiguration();
        return true;
        }
      });
  },

  getFilterRule: function (rule)
  {
    // Find the column
    var val, i, col, oper;
    var columns = jQuery("#grid").jqGrid ('getGridParam', 'colModel');
    for (i = 0; i < columns.length; i++)
    {
      if(columns[i].name === rule.field)
      {
        col = columns[i];
        break;
      }
    }
    if (col == undefined) return "";

    // Find operator
    for (var firstKey in $.jgrid.locales)
      var operands = $.jgrid.locales[firstKey].search.odata;
    for (i = 0; i < operands.length; i++)
      if (operands[i].oper == rule.op)
      {
        oper = operands[i].text;
        break;
      }
    if (oper == undefined) oper = rule.op;

    // Final result
    return col.label + ' ' + oper + ' "' + rule.data + '"';
  },

  getFilterGroup: function(group, first)
  {
    var s = "", index;

    if (!first) s = "(";

    if (group.groups !== undefined)
    {
      for (index = 0; index < group.groups.length; index++)
      {
        if (s.length > 1)
        {
          if (group.groupOp === "OR")
            s += " || ";
          else
            s += " && ";
        }
        s += grid.getFilterGroup(group.groups[index], false);
      }
    }

    if (group.rules !== undefined)
    {
      for (index = 0; index < group.rules.length; index++)
      {
        if (s.length > 1)
        {
          if (group.groupOp === "OR")
            s += " || ";
          else
            s += " && ";
        }
        s += grid.getFilterRule(group.rules[index]);
      }
    }

    if (!first) s += ")";

    if (s === "()")
      return ""; // ignore groups that don't have rules
    return s;
  },

  markSelectedRow: function(id)
  {
    var sel = jQuery("#grid").jqGrid('getGridParam','selarrrow').length;
    if (sel > 0)
    {
      $("#copy_selected").prop('disabled', false).addClass("bold");
      $("#delete_selected").prop('disabled', false).addClass("bold");
      $("#actions1").prop('disabled', false);
    }
    else
    {
      $("#copy_selected").prop('disabled', true).removeClass("bold");
      $("#delete_selected").prop('disabled', true).removeClass("bold");
      $("#actions1").prop('disabled', true);
    }
  },

  markAllRows: function()
  {
    if ($(this).is(':checked'))
    {
      $("#copy_selected").prop('disabled', false).addClass("bold");
      $("#delete_selected").prop('disabled', false).addClass("bold");
      $("#actions1").prop('disabled', false);
      $('.cbox').prop("checked", true);
    }
    else
    {
      $("#copy_selected").prop('disabled', true).removeClass("bold");
      $("#delete_selected").prop('disabled', true).removeClass("bold");
      $("#actions1").prop('disabled', true);
      $('.cbox').prop("checked", false);
    }
  },

  displayMode: function(m)
  {
    var url = (location.href.indexOf("#") != -1 ? location.href.substr(0,location.href.indexOf("#")) : location.href);
    if (location.search.length > 0)
      // URL already has arguments
      url = url.replace("&mode=table","").replace("&mode=graph","").replace("mode=table","").replace("mode=graph","") + "&mode=" + m;
    else if (url.charAt(url.length - 1) == '?')
      // This is the first argument for the URL, but we already have a question mark at the end
      url += "mode=" + m;
    else
      // This is the first argument for the URL
      url += "?mode=" + m;
    window.location.href = url;
  }
}

//----------------------------------------------------------------------------
// Code for Openbravo integration
//----------------------------------------------------------------------------

var openbravo = {
  IncrementalExport: function(grid, transactiontype) {
	// Collect all selected rows in the status 'proposed'
	  var sel = grid.jqGrid('getGridParam','selarrrow');
	  if (sel === null || sel.length == 0)
	    return;
	  var data = [];
	  for (var i in sel)
	  {
		  var r = grid.jqGrid('getRowData', sel[i]);
		  if (r.type === undefined)
			  r.type = transactiontype;
		  if (r.status == 'proposed')
		    data.push(r);
	  }
	  if (data == [])
		  return;

	  // Send to the server for upload into openbravo
     $('#timebuckets').modal('hide');
     $.jgrid.hideModal("#searchmodfbox_grid");
     $('#popup').html('<div class="modal-dialog">'+
           '<div class="modal-content">'+
             '<div class="modal-header">'+
               '<button type="button" class="close" data-dismiss="modal"><span aria-hidden="true" class="fa fa-times"></span></button>'+
               '<h4 class="modal-title">'+gettext("export")+'</h4>'+
             '</div>'+
             '<div class="modal-body">'+
               '<p>'+gettext("export selected records to openbravo")+'</p>'+
             '</div>'+
             '<div class="modal-footer">'+
               '<input type="submit" id="button_export" role="button" class="btn btn-danger pull-left" value="'+gettext('Confirm')+'">'+
               '<input type="submit" id="cancelbutton" role="button" class="btn btn-primary pull-right" data-dismiss="modal" value="'+gettext('Cancel')+'">'+
             '</div>'+
           '</div>'+
       '</div>' )
       .modal('show');
      $('#button_export').on('click', function() {
      $('#popup .modal-body p').html(gettext("connecting to openbravo..."));
	          var database = $('#database').val();
	          database = (database===undefined || database==='default') ? '' : '/' + database;
	          $.ajax({
	               url: database + "/openbravo/upload/",
	               data: JSON.stringify(data),
	               type: "POST",
	               contentType: "application/json",
	               success: function () {
             $('#popup .modal-body p').html(gettext("Export successful"))
             $('#cancelbutton').val(gettext('Close'));
             $('#button_export').removeClass("btn-primary").prop('disabled', true);
	                 // Mark selected rows as "approved" if the original status was "proposed".
	                 for (var i in sel)
	                 {
	                   var cur = grid.jqGrid('getCell', sel[i], 'status');
	                   if (cur == 'proposed')
	                     grid.jqGrid('setCell', sel[i], 'status', 'approved');
	                 }
	               },
	               error: function (result, stat, errorThrown) {
               fmts = ngettext("Error during export");
               $('#popup .modal-title').addClass('alert alert-danger').html(gettext("Error during export"));
             $('#popup .modal-body p').html(gettext("Error during export") + ':' + result.responseText);
             $('#button_export').text(gettext('retry'));
	               }
	           });
	  });
     $("#actions1").html($("#actionsul").children().first().text() + '  <span class="caret"></span>');
  }
};

//----------------------------------------------------------------------------
// Code for sending dashboard configuration to the server.
//----------------------------------------------------------------------------

var dashboard = {
  dragAndDrop: function() {

    $(".cockpitcolumn").each( function() {
      Sortable.create($(this)[ 0 ], {
        group: "test",
        animation: 100
      });
    });

      //stop: dashboard.save
    $(".panel-toggle").click(function() {
      var icon = $(this);
      icon.toggleClass("fa-minus fa-plus");
      icon.closest(".panel").find(".panel-body").toggle();
      });
    $(".panel-close").click(function() {
      $(this).closest(".panel").remove();
      dashboard.save();
      });
  },

  save : function(reload)
  {
    // Loop over all rows
    var results = [];
    $("[data-cockpit-row]").each(function() {
      var rowname = $(this).attr("data-cockpit-row");
      var cols = [];
      // Loop over all columns in the row
      $(".cockpitcolumn", this).each(function() {
        var width = 12;
        if ($(this).hasClass("col-md-12"))
          width = 12;
        else if ($(this).hasClass("col-md-11"))
          width = 11;
        else if ($(this).hasClass("col-md-10"))
          width = 10;
        else if ($(this).hasClass("col-md-9"))
          width = 9;
        else if ($(this).hasClass("col-md-8"))
          width = 8;
        else if ($(this).hasClass("col-md-7"))
          width = 7;
        else if ($(this).hasClass("col-md-6"))
          width = 6;
        else if ($(this).hasClass("col-md-5"))
          width = 5;
        else if ($(this).hasClass("col-md-4"))
          width = 4;
        else if ($(this).hasClass("col-md-3"))
          width = 3;
        else if ($(this).hasClass("col-md-2"))
          width = 2;
        // Loop over all widgets in the column
        var widgets = [];
        $("[data-cockpit-widget]", this).each(function() {
          widgets.push( [$(this).attr("data-cockpit-widget"),{}] );
        });
        cols.push( {'width': width, 'widgets': widgets});
      });
      if (cols.length > 0)
        results.push( {'rowname': rowname, 'cols': cols});
    });

    // Send to the server
    if (typeof url_prefix != 'undefined')
        var url = url_prefix + '/settings/';
      else
        var url = '/settings/';
    $.ajax({
      url: url,
      type: 'POST',
      contentType: 'application/json; charset=utf-8',
      data: JSON.stringify({"freppledb.common.cockpit": results}),
      success: function () {
        if ($.type(reload) === "string")
          window.location.href = window.location.href;
      },
      error: function (result, stat, errorThrown) {
        $('#popup').html('<div class="modal-dialog" style="width: auto">'+
            '<div class="modal-content">'+
            '<div class="modal-header">'+
              '<button type="button" class="close" data-dismiss="modal"><span aria-hidden="true" class="fa fa-times"></span></button>'+
              '<h4 class="modal-title">{% trans "Error saving report settings" %}</h4>'+
            '</div>'+
            '<div class="modal-body">'+
              '<p>'+result.responseText + "  " + stat + errorThrown+'</p>'+
            '</div>'+
            '<div class="modal-footer">'+
            '</div>'+
          '</div>'+
          '</div>' ).modal('show');
      }
      });
  },

  customize: function(rowname)
  {
    // Detect the current layout of this row
    var layout = "";
    $("[data-cockpit-row='" + rowname + "'] .cockpitcolumn").each(function() {
      if (layout != "")
        layout += " - ";
      if ($(this).hasClass("col-md-12"))
        layout += "100%";
      else if ($(this).hasClass("col-md-11"))
        layout += "92%";
      else if ($(this).hasClass("col-md-10"))
        layout += "83%";
      else if ($(this).hasClass("col-md-9"))
        layout += "75%";
      else if ($(this).hasClass("col-md-8"))
        layout += "67%";
      else if ($(this).hasClass("col-md-7"))
        layout += "58%";
      else if ($(this).hasClass("col-md-6"))
        layout += "50%";
      else if ($(this).hasClass("col-md-5"))
        layout += "42%";
      else if ($(this).hasClass("col-md-4"))
        layout += "33%";
      else if ($(this).hasClass("col-md-3"))
        layout += "25%";
      else if ($(this).hasClass("col-md-2"))
        layout += "17%";
      });

    var txt = '<div class="modal-dialog">' +
      '<div class="modal-content">' +
        '<div class="modal-header">' +
          '<button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>' +
          '<h4 class="modal-title">' + gettext("Customize a dashboard row") + '</h4>' +
        '</div>' +
      '<div class="modal-body">' +
        '<form class="form-horizontal">' +

         '<div class="form-group">' +
       '<label class="col-md-3 control-label" for="id_name">' + gettext("Name") + ':</label>' +
       '<div class="col-md-9">' +
       '<input id="id_name" class="form-control" type="text" value="' + rowname + '">' +
         '</div></div>' +

         '<div class="form-group">' +
       '<label class="col-md-3 control-label" for="id_layout2">' + gettext("Layout") + ':</label>' +
       '<div class="col-md-9 dropdown dropdown-submit-input">' +
     '<button class="btn btn-default dropdown-toggle" id="id_layout2" name="layout" type="button" data-toggle="dropdown" aria-haspopup="true">' +
       '<span id="id_layout">' + layout + '</span>&nbsp;<span class="caret"></span>' +
     '</button>' +
     '<ul class="dropdown-menu" aria-labelledby="id_layout" id="id_layoutul">' +
     '<li class="dropdown-header">' + gettext("Single column") + '</li>' +
     '<li><a onclick="dashboard.setlayout(this)">100%</a></li>' +
     '<li class="divider"></li>' +
     '<li class="dropdown-header">' + gettext("Two columns") + '</li>' +
     '<li><a onclick="dashboard.setlayout(this)">75% - 25%</a></li>' +
     '<li><a onclick="dashboard.setlayout(this)">67% - 33%</a></li>' +
     '<li><a onclick="dashboard.setlayout(this)">50% - 50%</a></li>' +
     '<li><a onclick="dashboard.setlayout(this)">33% - 67%</a></li>' +
     '<li><a onclick="dashboard.setlayout(this)">25% - 75%</a></li>' +
     '<li class="divider"></li>' +
     '<li class="dropdown-header">' + gettext("Three columns") + '</li>' +
     '<li><a onclick="dashboard.setlayout(this)">50% - 25% - 25%</a></li>' +
     '<li><a onclick="dashboard.setlayout(this)">33% - 33% - 33%</a></li>' +
     '<li class="divider"></li>' +
     '<li class="dropdown-header">' + gettext("Four columns") + '</li>' +
     '<li><a onclick="dashboard.setlayout(this)">25% - 25% - 25% - 25%</a></li>' +
     '</ul></div>' +
       '</div>' +

         '<div class="form-group">' +
       '<label class="col-md-3 control-label" for="id_widget2">' + gettext("Add widget") + ':</label>' +
       '<div class="col-md-9 dropdown dropdown-submit-input">' +
     '<button class="btn btn-default dropdown-toggle" id="id_widget2" type="button" data-toggle="dropdown">' +
     '<span id="id_widget">-</span>&nbsp;<span class="caret"></span>' +
     '</button>' +
     '<ul class="dropdown-menu col-sm-9" aria-labelledby="id_widget2" id="id_widgetul">';

       var numwidgets = hiddenwidgets.length;
       for (var i = 0; i < numwidgets; i++)
         txt += '<li><a onclick="dashboard.setwidget(' + i + ')">' + hiddenwidgets[i][1] + '</a></li>';

       txt +=
     '</ul></div><span id="newwidgetname" style="display:none"></span>' +
       '</div>' +

     '</form></div>' +
     '<div class="modal-footer">' +
       '<input type="submit" role="button" onclick=\'dashboard.saveCustomization("' + rowname + '")\' class="btn btn-danger pull-left" value="' + gettext('Save') + '">' +
       '<input type="submit" role="button" onclick=\'dashboard.deleteRow("' + rowname + '")\' class="btn btn-danger pull-left" value="' + gettext('Delete') + '">' +
       '<input type="submit" role="button" onclick=\'$("#popup").modal("hide")\' class="btn btn-primary pull-right" data-dismiss="modal" value="' + gettext('Cancel') + '">' +
       '<input type="submit" role="button" onclick=\'dashboard.addRow("' + rowname + '", false)\' class="btn btn-primary pull-right" value="' + gettext('Add new below') + '">' +
       '<input type="submit" role="button" onclick=\'dashboard.addRow("' + rowname + '", true)\' class="btn btn-primary pull-right" value="' + gettext('Add new above') + '">' +
     '</div>' +

     '</div></div></div>';

      $('#popup').html(txt).modal('show');
  },

  setlayout: function(elem) {
    $("#id_layout").text($(elem).text());
  },

  setwidget: function(idx) {
    $("#id_widget").text(hiddenwidgets[idx][1]);
    $("#newwidgetname").text(hiddenwidgets[idx][0]);
  },

  saveCustomization: function(rowname) {
	// Update the name
    var newname = $("#id_name").val();
    if (rowname != newname)
    {
      // Make sure name is unique
      var cnt = 2;
      while ($("[data-cockpit-row='" + newname + "']").length > 1)
        newname = $("#id_name").val() + ' - ' + (cnt++);

      // Update
      $("[data-cockpit-row='" + rowname + "'] .col-md-11 h1").text(newname);
      $("[data-cockpit-row='" + rowname + "'] h1 button").attr("onclick", "dashboard.customize('" + newname + "')");
      $("[data-cockpit-row='" + rowname + "']").attr("data-cockpit-row", newname);
    }

    // Update the layout
    var newlayout = $("#id_layout").text().split("-");
    var colindex = 0;
    var lastcol = null;
    // Loop over existing columns
    $("[data-cockpit-row='" + rowname + "'] .cockpitcolumn").each(function() {
      if (colindex < newlayout.length)
      {
        // Resize existing column
        lastcol = this;
        $(this).removeClass("col-md-1 col-md-2 col-md-3 col-md-4 col-md-5 col-md-6 col-md-7 col-md-8 col-md-9 col-md-10 col-md-11 col-md-12");
        $(this).addClass("col-md-" + Math.round(0.12 * parseInt(newlayout[colindex])));
      }
      else
      {
        // Remove this column, after moving all widgets to the previous column
        $("[data-cockpit-widget]", this).appendTo(lastcol);
        $(this).remove();
      }
      colindex++;
    });
    while(colindex < newlayout.length)
    {
      // Adding extra columns
      lastcol = $('<div class="cockpitcolumn col-md-' + Math.round(0.12 * parseInt(newlayout[colindex])) + ' col-sm-12"></div>').insertAfter(lastcol);
      colindex++;
    }

    // Adding new widget
    var newwidget = $("#newwidgetname").text();
    if (newwidget != '')
    {
      $('<div class="panel panel-default" data-cockpit-widget="' + newwidget + '"></div>').appendTo(lastcol);
      dashboard.save("true"); // Force reload of the page
    }
    else
      dashboard.save();

    // Almost done
    dashboard.dragAndDrop();
    $('#popup').modal('hide');
  },

  deleteRow: function(rowname) {
    $("[data-cockpit-row='" + rowname + "']").remove();
    dashboard.save();
    $('#popup').modal('hide');
  },

  addRow: function(rowname, position_above) {
	// Make sure name is unique
	var newname = $("#id_name").val();
	var cnt = 2;
	while ($("[data-cockpit-row='" + newname + "']").length > 1)
      newname = $("#id_name").val() + ' - ' + (cnt++);

    // Build new content
    var newelements = '<div class="row" data-cockpit-row="' + newname + '">' +
      '<div class="col-md-11"><h1 style="float: left">' + newname + '</h1></div>' +
      '<div class="col-md-1"><h1 class="pull-right">' +
      '<button class="btn btn-xs btn-primary" onclick="dashboard.customize(\'' + newname + '\')" data-toggle="tooltip" data-placement="top" data-original-title="' + gettext("customize") + '"><span class="fa fa-wrench"></span></button>' +
      '</h1></div>' +
      '</div>' +
      '<div class="row" data-cockpit-row="' + newname + '">';
    var newlayout = $("#id_layout").text().split("-");
    var newwidget = $("#newwidgetname").text();
    for (var i = 0; i < newlayout.length; i++)
    {
      newelements += '<div class="cockpitcolumn col-md-' + Math.round(0.12 * parseInt(newlayout[i])) + ' col-sm-12">';
      if (i == 0 && newwidget != '')
        newelements += '<div class="panel panel-default" data-cockpit-widget="' + newwidget + '"></div>';
      newelements += '</div>';
    }
    newelements += '</div></div>';

    // Insert in page
    if (position_above)
      $("[data-cockpit-row='" + rowname + "']").first().before($(newelements));
    else
      $("[data-cockpit-row='" + rowname + "']").last().after($(newelements));

    // Almost done
    if (newwidget != '')
      // Force reload of the page when adding a widget
      dashboard.save("true");
    else
      dashboard.save();
    dashboard.dragAndDrop();
    $('#popup').modal('hide');
  }

}

//----------------------------------------------------------------------------
// Code for handling the menu bar, context menu and active button.
//----------------------------------------------------------------------------

var activeButton = null;
var contextMenu = null;

$(function() {

  // Install code executed when you click on a menu button
  $(".menuButton").click( function(event) {
    // Get the target button element
    var button = $(event.target);
    var menu = button.next("div");

    // Blur focus from the link to remove that annoying outline.
    button.blur();

    // Reset the currently active button, if any.
    if (activeButton) {
      activeButton.removeClass("menuButtonActive");
      activeButton.next("div").css('visibility', "hidden");
    }

    // Activate this button, unless it was the currently active one.
    if (button != activeButton)
    {
      // Update the button's style class to make it look like it's depressed.
      button.addClass("menuButtonActive");

      // Position the associated drop down menu under the button and show it.
      var pos = button.position();
      menu.css({
        left: pos.left + "px",
        top: (pos.top + button.outerHeight() + 3) + "px",
        visibility: "visible"
        });
      activeButton = button;
    }
    else
      activeButton = null;
  });

  $('.menuButton').mouseenter( function(event) {
    // If another button menu is active and we move the mouse into a new menu button,
    // we make this one active instead.
    if (activeButton != null && activeButton != $(event.target))
      $(event.target).click();
  });

  // Send django's CRSF token with every POST request to the same site
  $(document).ajaxSend(function(event, xhr, settings) {
    if (!/^(GET|HEAD|OPTIONS|TRACE)$/.test(settings.type) && sameOrigin(settings.url))
      xhr.setRequestHeader("X-CSRFToken", getToken());
    });

  // Never cache ajax results
  $.ajaxSetup({ cache: false });

  // Autocomplete search functionality
  var database = $('#database').attr('name');
  database = (database===undefined || database==='default') ? '' : '/' + database;

  var searchsource = new Bloodhound({
    datumTokenizer: Bloodhound.tokenizers.obj.whitespace('value'),
    queryTokenizer: Bloodhound.tokenizers.whitespace,
    //prefetch: '/search/',
    remote: {
      url: database+'/search/?term=%QUERY',
      wildcard: '%QUERY'
    }
  });
  $('#search').typeahead({minLength: 2}, {
    limit:100,
    highlight: true,
    name: 'search',
    display: 'value',
    source: searchsource,
    templates: {
      suggestion: function(data){
        if (data.value === null)
          return '<span><p style="margin-top: 5px; margin-bottom: 1px;">'+data.label+'</p><li  role="separator" class="divider"></li></span>';
        else
          return '<li><a href="'+ database + data.url + admin_escape(data.value) + '/" >' + data.value + '</a></li>';
      },
    }
  });

});


// Capture mouse clicks on the page so any active menu can be deactivated.
$(document).mousedown(function (event) {

  if (contextMenu && $(event.target).parent('.ui-menu-item').length < 1)
  {
    // Hide any context menu
    contextMenu.css('display', 'none');
    contextMenu = null;
  }

  // We clicked on a context menu. Display that now.
  if ($(event.target).hasClass('context'))
  {
    // Find the id of the menu to display
    contextMenu = $('#' + $(event.target).attr('role') + "context");

    // Get the entity name.
    if ($(event.target).hasClass('cross'))
    {
      var item = $(event.target).closest("tr.jqgrow")[0].id;
      item = admin_escape(item);
      var params = jQuery("#grid").jqGrid ('getGridParam', 'colModel')[jQuery.jgrid.getCellIndex($(event.target).closest("td,th"))];
      params['value'] = item;
    }
    else
    {
      var item = $(event.target).parent().text();
      item = admin_escape(item);
      var params = {value: item};
    }

    // Build the URLs for the menu
    contextMenu.find('a').each( function() {
      $(this).attr('href', $(this).attr('id').replace(/{\w+}/g, function(match, number) {
        var key = match.substring(1,match.length-1);
        return key in params ? params[key] : match;
        }))
    });

    // Display the menu at the right location
    $(contextMenu).css({
      left: event.pageX,
      top: event.pageY,
      display: 'block'
      });
    event.preventDefault();
    event.stopImmediatePropagation();
  }

  // If there is no active button, exit.
  if (!activeButton || event.target == activeButton) return;

  // If the element is not part of a menu, hide the menu
  if ($(event.target).parent('.ui-menu-item').length < 1) {
    activeButton.removeClass("menuButtonActive");
    activeButton.next("div").css('visibility', "hidden");
    activeButton = null;
  }
});


//----------------------------------------------------------------------------
// Return the value of the csrf-token
//----------------------------------------------------------------------------

function getToken()
{
  var allcookies = document.cookie.split(';');
  for (var i = allcookies.length; i >= 0; i-- )
    if (jQuery.trim(allcookies[i]).indexOf("csrftoken=") == 0)
      return jQuery.trim(jQuery.trim(allcookies[i]).substr(10));
  return 'none';
}


//----------------------------------------------------------------------------
// Check whether a URL is on the same domain as the current location or not.
// We use it to avoid send the CSRF-token to ajax requests submitted to other
// sites - for security reasons.
//----------------------------------------------------------------------------

function sameOrigin(url) {
    // URL could be relative or scheme relative or absolute
    var host = document.location.host; // host + port
    var protocol = document.location.protocol;
    var sr_origin = '//' + host;
    var origin = protocol + sr_origin;
    // Allow absolute or scheme relative URLs to same origin
    return (url == origin || url.slice(0, origin.length + 1) == origin + '/') ||
        (url == sr_origin || url.slice(0, sr_origin.length + 1) == sr_origin + '/') ||
        // or any other URL that isn't scheme relative or absolute i.e relative.
        !(/^(\/\/|http:|https:).*/.test(url));
}


//----------------------------------------------------------------------------
// Display import dialog for CSV-files
//----------------------------------------------------------------------------

function import_show(url)
{
  $('#timebuckets').modal('hide');
  $.jgrid.hideModal("#searchmodfbox_grid");
  $('#popup').html('<div class="modal-dialog">'+
      '<div class="modal-content">'+
        '<div class="modal-header">'+
          '<button type="button" class="close" data-dismiss="modal" aria-label="Close"><span aria-hidden="true">&times;</span></button>'+
          '<h4 class="modal-title">'+ gettext("Import CSV or Excel file") +'</h4>'+
        '</div>'+
        '<div class="modal-body">'+
    '<form id="uploadform">' +
            '<p>'+gettext('Load an Excel file or a CSV-formatted text file.') + '<br/>' +
    gettext('The first row should contain the field names.') + '<br/><br/>' +
            '</p>'+
            '<input type="checkbox"  autocomplete="off" name="erase" value="yes"/>&nbsp;&nbsp;' + gettext('First delete all existing records AND ALL RELATED TABLES') + '<br/><br/>' +
            gettext('Data file') + ':<input type="file" id="csv_file" name="csv_file"/>'+
          '</form>' +
          '<br/><div style="margin: 5px 0"><textarea id="uploadResponse" rows="10" style="display: none; width:100%; background-color: inherit; border: none" readonly="readonly"></textarea></div>'  +
        '</div>'+
        '<div class="modal-footer">'+
            '<input type="submit" id="importbutton" role="button" class="btn btn-danger pull-left" value="'+gettext('Import')+'">'+
            '<input type="submit" id="cancelbutton" role="button" class="btn btn-primary pull-right" data-dismiss="modal" value="'+gettext('Cancel')+'">'+
        '</div>'+
      '</div>'+
    '</div>' )
  .modal('show');
  $('#importbutton').on('click', function() {
            if ($("#csv_file").val() == "") return;
            $('#uploadResponse').css('display','block');
            $.ajax({
              type: 'post',
              url: typeof(url) != 'undefined' ? url : '',
              cache: false,
              data: new FormData($("#uploadform")[0]),
              success: function (data) {
                var el = $('#uploadResponse');
                el.val(data);
                el.scrollTop(el[0].scrollHeight - el.height());
              },
              xhrFields: {
                onprogress: function (e) {
                  var el = $('#uploadResponse');
                  el.val(e.currentTarget.response);
                  el.scrollTop(el[0].scrollHeight - el.height());
                }
              },
              processData: false,
              contentType: false
              });
          }
  )
}


//----------------------------------------------------------------------------
// This function returns all arguments in the current URL as a dictionary.
//----------------------------------------------------------------------------

function getURLparameters()
{

  if (window.location.search.length == 0) return {};
  var params = {};
  jQuery.each(window.location.search.match(/^\??(.*)$/)[1].split('&'), function(i,p){
    p = p.split('=');
    p[1] = unescape(p[1]).replace(/\+/g,' ');
    params[p[0]] = params[p[0]]?((params[p[0]] instanceof Array)?(params[p[0]].push(p[1]),params[p[0]]):[params[p[0]],p[1]]):p[1];
  });
  return params;
}


//----------------------------------------------------------------------------
// Dropdown list to select the model.
//----------------------------------------------------------------------------

function selectDatabase()
{
  // Find new database and current database
  var db = $(this).text();
  var cur = $('#database').attr('name');

  // Change the location
  if (cur == db)
    return;
  else if (cur == 'default')
  {
    if (window.location.pathname == '/')
      window.location.href = "/"+db+"/";
    else
      window.location.href = window.location.href.replace(window.location.pathname, "/"+db+window.location.pathname);
  }
  else if (db == 'default')
    window.location.href = window.location.href.replace("/"+cur+"/", "/");
  else
    window.location.href = window.location.href.replace("/"+cur+"/", "/"+db+"/");
}


//----------------------------------------------------------------------------
// Jquery utility function to bind an event such that it fires first.
//----------------------------------------------------------------------------

$.fn.bindFirst = function(name, fn) {
  // bind as you normally would
  // don't want to miss out on any jQuery magic
  this.on(name, fn);

  // Thanks to a comment by @Martin, adding support for
  // namespaced events too.
  this.each(function() {
    var handlers = $._data(this, 'events')[name.split('.')[0]];
    // take out the handler we just inserted from the end
    var handler = handlers.pop();
    // move it at the beginning
    handlers.splice(0, 0, handler);
  });
};


//
// Graph functions
//

var graph = {

  header: function()
  {
    var el = $("#grid_graph");
    el.html("");
    var bucketwidth = (el.width() - 50) / numbuckets;
    var svg = d3.select(el.get(0)).append("svg");
    svg.attr('height','15px');
    svg.attr('width', el.width());
    var w = 50 + bucketwidth / 2;
    var wt = w;
    for (var i in timebuckets)
    {
      if (wt <= w)
      {
        var t = svg.append('text')
          .attr('class','svgheadertext')
          .attr('x', w)
          .attr('y', '12')
          .attr('class','graphheader')
          .text(timebuckets[i]['name']);
        wt = w + t.node().getComputedTextLength() + 12;
      }
      w += bucketwidth;
    }
  },

  showTooltip: function(txt)
  {
    // Find or create the tooltip div
    var tt = d3.select("#tooltip");
    if (tt.empty())
      tt = d3.select("body")
        .append("div")
        .attr("id", "tooltip")
        .attr("role", "tooltip")
        .attr("class", "popover fade right in")
        .style("position", "absolute");

    // Update content and display
    tt.html('' + txt)
      .style('display', 'block');
    graph.moveTooltip();
  },

  hideTooltip: function()
  {
    d3.select("#tooltip").style('display', 'none');
    d3.event.stopPropagation();
  },

  moveTooltip: function()
  {
    var xpos = d3.event.pageX + 5;
    var ypos = d3.event.pageY - 28;
    var xlimit = $(window).width() - $("#tooltip").width() - 20;
    var ylimit = $(window).height() - $("#tooltip").height() - 20;
    if (xpos > xlimit)
    {
      // Display tooltip under the mouse
      xpos = xlimit;
      ypos = d3.event.pageY + 5;
    }
    if (ypos > ylimit)
      // Display tooltip above the mouse
      ypos = d3.event.pageY - $("#tooltip").height() - 25;
    d3.select("#tooltip")
      .style({
        'left': xpos + "px",
        'top': ypos + "px"
        });
    d3.event.stopPropagation();
  },

  miniAxis: function(s)
  {
    var sc = this.scale().range();
    var dm = this.scale().domain();
    // Draw the scale line
    s.append("path")
     .attr("class", "domain")
     .attr("d", "M-10 0 H0 V" + (sc[0]-2) + " H-10");
    // Draw the maximum value
    s.append("text")
     .attr("x", -2)
     .attr("y", 13) // Depends on font size...
     .attr("text-anchor", "end")
     .text(Math.round(dm[1], 0));
    // Draw the minimum value
    s.append("text")
    .attr("x", -2)
    .attr("y", sc[0] - 5)
    .attr("text-anchor", "end")
    .text(Math.round(dm[0], 0));
  }
};

//
// Gantt chart functions
//

var gantt = {

  // Used to follow the mous when dragging the timeline
  startmousemove: null,
  resizing: null,

  // Height of the blocks
  rowsize: 25,

  header : function ()
  {
    // "scaling" stores the number of pixels available to show a day.
    var scaling = 86400000 / (viewend.getTime() - viewstart.getTime()) * $("#jqgh_grid_operationplans").width();
    var result = [
      '<svg xmlns="http://www.w3.org/2000/svg" version="1.1" width="100%" height="34px">',
      '<line class="time" x1="0" y1="17" x2="' + $("#jqgh_grid_operationplans").width() + '" y2="17"/>'
      ];
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
        result.push('<text class="svgheadertext" x="' + Math.floor((x1+x2)/2) + '" y="31">' + moment(bucketstart).format("MMM") + '</text>');
        if (bucketstart.getMonth() % 3 == 0)
        {
          var quarterend = new Date(bucketstart.getFullYear(), bucketstart.getMonth()+3, 1);
          x2 = (quarterend.getTime() - viewstart.getTime()) / 86400000 * scaling;
          var quarter = Math.floor((bucketstart.getMonth()+3)/3);
          result.push('<line class="time" x1="' + Math.floor(x1) + '" y1="0" x2="' + Math.floor(x1) + '" y2="34"/>');
          result.push('<text class="svgheadertext" x="' + Math.floor((x1+x2)/2) + '" y="13">' + bucketstart.getFullYear() + " Q" + quarter + '</text>');
        }
        else
          result.push('<line class="time" x1="' + Math.floor(x1) + '" y1="17" x2="' + Math.floor(x1) + '" y2="34"/>');
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
        result.push('<line class="time" x1="' + Math.floor(x) + '" y1="17" x2="' + Math.floor(x) + '" y2="34"/>');
        result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling*3.5) + '" y="31">' + moment(bucketstart).format("MM-DD") + '</text>');
        x = x + scaling*7;
        bucketstart.setTime(bucketstart.getTime() + 86400000 * 7);
      }
      bucketstart = new Date(viewstart.getFullYear(), viewstart.getMonth(), 1);
      while (bucketstart < viewend)
      {
        x1 = (bucketstart.getTime() - viewstart.getTime()) / 86400000 * scaling;
        bucketend = new Date(bucketstart.getFullYear(), bucketstart.getMonth()+1, 1);
        x2 = (bucketend.getTime() - viewstart.getTime()) / 86400000 * scaling;
        result.push('<line class="time" x1="' + Math.floor(x1) + '" y1="0" x2="' + Math.floor(x1) + '" y2="17"/>');
        result.push('<text class="svgheadertext" x="' + Math.floor((x1+x2)/2) + '" y="13">' + moment(bucketstart).format("MMM YY") + '</text>');
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
        result.push('<line class="time" x1="' + Math.floor(x) + '" y1="17" x2="' + Math.floor(x) + '" y2="34"/>');
        result.push('<text class="svgheadertext" x="' + (x + scaling*7.0/2.0) + '" y="31">' + moment(bucketstart).format("YY-MM-DD") + '</text>');
        x = x + scaling*7.0;
        bucketstart.setTime(bucketstart.getTime() + 86400000 * 7);
      }
      bucketstart = new Date(viewstart.getFullYear(), viewstart.getMonth(), 1);
      while (bucketstart < viewend)
      {
        x1 = (bucketstart.getTime() - viewstart.getTime()) / 86400000 * scaling;
        bucketend = new Date(bucketstart.getFullYear(), bucketstart.getMonth()+1, 1);
        x2 = (bucketend.getTime() - viewstart.getTime()) / 86400000 * scaling;
        result.push('<line class="time" x1="' + Math.floor(x1) + '" y1="0" x2="' + Math.floor(x1) + '" y2="17"/>');
        result.push('<text class="svgheadertext" x="' + Math.floor((x1+x2)/2) + '" y="13">' + moment(bucketstart).format("MMM YY") + '</text>');
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
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="0" x2="' + Math.floor(x) + '" y2="34"/>');
          result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling*7/2) + '" y="13">' + moment(bucketstart).format("YY-MM-DD") + '</text>');
        }
        else
        {
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="17" x2="' + Math.floor(x) + '" y2="34"/>');
        }
        result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling/2) + '" y="31">' + moment(bucketstart).format("DD") + '</text>');
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
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="0" x2="' + Math.floor(x) + '" y2="34"/>');
          result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling*7/2) + '" y="13">' + moment(bucketstart).format("YY-MM-DD") + '</text>');
        }
        else
        {
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="17" x2="' + Math.floor(x) + '" y2="34"/>');
        }
        result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling/2) + '" y="31">' + moment(bucketstart).format("DD MM") + '</text>');
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
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="0" x2="' + Math.floor(x) + '" y2="34"/>');
          result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling*3.5) + '" y="13">' + moment(bucketstart).format("YY-MM-DD") + '</text>');
        }
        else
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="17" x2="' + Math.floor(x) + '" y2="34"/>');
        result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling/2) + '" y="31">' + moment(bucketstart).format("ddd DD MMM") + '</text>');
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
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="0" x2="' + Math.floor(x) + '" y2="34"/>');
          result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling/2) + '" y="13">' + moment(bucketstart).format("ddd YY-MM-DD") + '</text>');
        }
        else
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="17" x2="' + Math.floor(x) + '" y2="34"/>');
        result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling/48) + '" y="31">' + bucketstart.getHours() + '</text>');
        x = x + scaling/24;
        bucketstart.setTime(bucketstart.getTime() + 3600000);
      }
    }
    result.push( '</svg>' );
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
  },

  reset: function()
  {
    var scale = $("#jqgh_grid_operationplans").width() / 10000;
    viewstart = new Date(horizonstart.getTime());
    viewend = new Date(horizonend.getTime());
    $('.transformer').each(function() {
      var layers = $(this).attr("title");
      $(this).attr("transform", "scale(" + scale + ",1) translate(0," + ((layers-1)*gantt.rowsize+3) + ")");
      });
    gantt.header();
  },

  redraw: function()
  {
    // Determine the conversion between svg units and the screen
    var scale = (horizonend.getTime() - horizonstart.getTime())
       / (viewend.getTime() - viewstart.getTime())
       * $("#jqgh_grid_operationplans").width() / 10000;
    $('.transformer').each(function() {
      var layers = $(this).attr("title");
      $(this).attr("transform", "scale(" + scale + ",1) translate(0," + ((layers-1)*gantt.rowsize+3) + ")");
      });
    gantt.header();
  },

  zoom: function(zoom_in_or_out, move_in_or_out)
  {
    // Determine the window to be shown. Min = 1 day. Max = 3 years.
    var delta = Math.min(1095,Math.max(1,Math.ceil((viewend.getTime() - viewstart.getTime()) / 86400000.0 * zoom_in_or_out)));
    // Determine the start and end date global variables.
    viewstart.setTime(viewstart.getTime() + move_in_or_out);
    viewend.setTime(viewstart.getTime() + delta * 86400000);
    // Determine the conversion between svg units and the screen
    var scale = (horizonend.getTime() - horizonstart.getTime()) / (delta * 864000000) * $("#jqgh_grid_operationplans").width() / 1000;
    var offset = (horizonstart.getTime() - viewstart.getTime()) / (horizonend.getTime() - horizonstart.getTime()) * 10000;
    // Transform all svg elements
    $('.transformer').each(function() {
      var layers = $(this).attr("title");
      $(this).attr("transform", "scale(" + scale + ",1) translate(" + offset + "," + ((layers-1)*gantt.rowsize+3) + ")");
      });
    // Redraw the header
    gantt.header();
  }
}


var tour = {

  autoplay: 0,
  tooltip: null,
  chapter: 0,
  step: 0,
  timeout: null,


  start: function (args)
  {
    // Parse the arguments
    var splitargs = args.split(",");
    tour.chapter = parseInt(splitargs[0]);
    tour.step = parseInt(splitargs[1]);
    tour.autoplay = parseInt(splitargs[2]);
    // Load and execute the tutorial
    jQuery.ajax( {
        url: "/static/js/i18n/tour.en.js",
        dataType: "script",
        cache: true
      })
      .success( tour.init )
      .fail( function() {
        console.log('Error loading the tutorial: ' + arguments[2].toString());
      });
  },

  init: function()
  {
     // Display the main dialog of the tour

    $('#timebuckets').modal('hide');
    $.jgrid.hideModal("#searchmodfbox_grid");

    $('#popup').removeClass("fade in").addClass("tourguide").html('<div class="modal-dialog" id="tourModal" role="dialog" style="width: 390px; position: absolute; bottom: 10px; left: auto; right: 15px;">'+
        '<div class="modal-content">'+
        '<div class="modal-header">'+
          '<h4 id="modalTitle" class="modal-title alert alert-info">'+ gettext("Guided tour") +
          '<button type="button" id="tourcancelbutton" class="close" data-dismiss="modal" aria-hidden="true">×</button>'+'</h4>'+
        '</div>'+
        '<div class="modal-body" id="tourmodalbody" style="padding-bottom:20px;">'+
            tourdata[tour.chapter]['description']+
        '</div>'+
        '<div class="modal-footer"><div class="btn-group control-form" role="group" aria-label="tour buttons">'+
          '<button type="submit" id="tourprevious" role="button" class="btn btn-primary">'+'<span class="fa fa-step-backward"></span>&nbsp;'+gettext('Previous')+'</button>'+
          '<button type="submit" id="playbutton" role="button" class="btn btn-primary">'+ gettext(tour.autoplay === 0 ? 'Play' : 'Pause')+ '&nbsp;<span class= ' + ((tour.autoplay === 0) ? '"fa fa-play"' : '"fa fa-pause"') + '></span></button>'+
          '<button type="submit" id="tournext" role="button" class="btn btn-primary">'+gettext('Next')+'&nbsp;<span class="fa fa-step-forward"></span></button>'+
        '</div></div>'+
      '</div>'+
    '</div>' )
    .modal({
      backdrop: 'static',
      keyboard: false
    })
    .modal('show');
    $('#tourmodalbody').append( '<div id="tour" style="padding-bottom:20px; display:none">' +
         tourdata[tour.chapter]['description']  + '<br/><br/><br/></div>');
    $('#tourprevious').on('click', function() {
      tour.prev();
    });
    $('#playbutton').on('click', function() {
      tour.toggleAutoplay();
    });
    $('#tournext').on('click', function() {
      tour.next();
    });
    $('#tourcancelbutton').on('click', function() {
          $('#tourtooltip').remove();
      $('#tourModal').modal('hide');
      });

     // Create the tooltip
     tour.tooltip = $('<div>',{id:'tourtooltip', class:'popover', html:'<div class="popover" role="tooltip" style="margin-top:10px;"><div class="arrow"></div><h3 class="popover-title"></h3><div class="popover-content"></div></div>'})
     .css({
        'placement': 'top','display': 'none', 'overflow': 'visible'
     });
     $("body").append(tour.tooltip);

     // Show the first step
     tour.showStep();
  },

  next: function()
  {
    tour.step++;
    if (tour.step >= tourdata[tour.chapter]['steps'].length)
    {
      tour.chapter++;
      if (tour.chapter < tourdata.length)
        tour.step = 0;
      else if (tour.autoplay == 2)
      {
        // Restart from the beginning
        tour.step = 0;
        tour.chapter = 0;
      }
      else
      {
        // Stop at the last step
        if (tour.autoplay == 1) tour.toggleAutoplay();
        tour.chapter--;
        tour.step--;
        return;
      }
    }
    tour.showStep();
  },

  prev: function()
  {
    tour.step--;
    if (tour.step < 0)
    {
      tour.chapter--;
      if (tour.chapter < 0)
      {
        // Stay at the very first step
        tour.step = 0;
        tour.chapter = 0;
        return;
      }
      else
        tour.step = tourdata[tour.chapter]['steps'].length - 1;
    }
    tour.showStep();
  },

  showStep: function()
  {
    var stepData = tourdata[tour.chapter]['steps'][tour.step];
    // Switch url if required
    var prefix = $('#database').attr('name');
    if (prefix && prefix != "default")
    {
      if (location.pathname != "/" + prefix + stepData['url'])
      {
        window.location.href = "/" + prefix + stepData['url'] + "?tour=" + tour.chapter + "," + tour.step + "," + tour.autoplay;
        return;
      }
    }
    else
    {
      if (location.pathname != stepData['url'])
      {
        window.location.href = stepData['url'] + "?tour=" + tour.chapter + "," + tour.step + "," + tour.autoplay;
        return;
      }
    }
    // Callback
    if ('beforestep' in stepData)
      eval(stepData['beforestep']);
    // Display the tooltip
    tour.tooltip.html(stepData['description']);
    var tooltipPos = (typeof stepData.position == 'undefined') ? 'BL' : stepData['position'];
    var pos = tour.getTooltipPosition(tooltipPos, stepData['element']);
    tour.tooltip.css({ 'top': pos.top+'px', 'left': pos.left+'px' });
    tour.tooltip.show('fast');
    // Update tour dialog
    $('#tour').html(tourdata[tour.chapter]['description'] + '<br/><br/>' + (tour.step+1) + " " + gettext("out of") + " " + tourdata[tour.chapter]['steps'].length);
    // Previous button
    if (tour.chapter == 0 && tour.step == 0)
      $("#tourprevious").prop('disabled', true);
    else
      $("#tourprevious").prop('disabled', false);
    // Next button
    if ((tour.chapter >= tourdata.length-1) && (tour.step >= tourdata[tour.chapter]['steps'].length-1))
      {$("#tournext").prop('disabled', true);
    console.log("chapter");}
    else
      $("#tournext").prop('disabled', false);
    // Autoplay
    if (tour.autoplay)
      tour.timeout = setTimeout(tour.next, tourdata[tour.chapter]['delay'] * 1000);
    // Callback
    if ('afterstep' in stepData)
      eval(stepData['afterstep']);
  },

  toggleAutoplay: function()
  {
    if (tour.autoplay > 0)
    {
      $("#playbutton").html(gettext('Play')+'&nbsp;<span class="fa fa-play"></span>');
      tour.autoplay = 0;
      clearTimeout(tour.timeout);
      tour.timeout = null;
    }
    else
    {
      $("#playbutton").html(gettext('Pause')+'&nbsp;<span class="fa fa-pause"></span>');
      tour.autoplay = 1;
      tour.next();
    }
  },

  getTooltipPosition: function(pos, elementselector)
  {
    var element = $(elementselector);
    if (element.length == 0)
    {
      console.log("Warning: Tour refers to nonexisting element '" + elementselector + "'");
      return { 'left'  : 100, 'top' : 100 };
    }
    var position;
    var ew = element.outerWidth();
    var eh = element.outerHeight();
    var offset = element.offset();
    var el = offset.left;
    var et = offset.top;
    var tw = tour.tooltip.width() + parseInt(tour.tooltip.css('padding-left')) + parseInt(tour.tooltip.css('padding-right'));
    var th = tour.tooltip.height() + parseInt(tour.tooltip.css('padding-top')) +  + parseInt(tour.tooltip.css('padding-bottom'));

    $('.tourArrow').remove();
    var upArrow = $('<div class="tourArrow"></div>').css({ 'position' : 'absolute', 'display' : 'block', 'width' : '0', 'height' : '0', 'border-left' : '9px solid transparent', 'border-right' : '9px solid transparent', 'border-bottom' : '9px solid red'});
    var downArrow = $('<div class="tourArrow"></div>').css({ 'position' : 'absolute', 'display' : 'block', 'width' : '0', 'height' : '0', 'border-left' : '9px solid transparent', 'border-right' : '9px solid transparent', 'border-top' : '9px solid red'});
    var rightArrow = $('<div class="tourArrow"></div>').css({ 'position' : 'absolute', 'display' : 'block', 'width' : '0', 'height' : '0', 'border-top' : '9px solid transparent', 'border-bottom' : '9px solid transparent', 'border-left' : '9px solid red'});
    var leftArrow = $('<div class="tourArrow"></div>').css({ 'position' : 'absolute', 'display' : 'block', 'width' : '0', 'height' : '0', 'border-top' : '9px solid transparent', 'border-bottom' : '9px solid transparent', 'border-right' : '9px solid red'});
    switch (pos) {
      case 'BL' :
        position = { 'left'  : el, 'top' : et + eh + 10 };
        upArrow.css({ top: '-9px', left: '48%' });
        tour.tooltip.prepend(upArrow);
        break;

      case 'BR' :
        position = { 'left'  : el + ew - tw, 'top' : et + eh + 10 };
        upArrow.css({ top: '-9px', left: '48%' });
        tour.tooltip.prepend(upArrow);
        break;

      case 'TL' :
        position = { 'left'  : el, 'top' : (et - th) -10 };
        downArrow.css({ top: th, left: '48%' });
        tour.tooltip.append(downArrow);
        break;

      case 'TR' :
        position = { 'left'  : (el + ew) - tw, 'top' : et - th -10 };
        downArrow.css({ top: th, left: '48%' });
        tour.tooltip.append(downArrow);
        break;

      case 'RT' :
        position = { 'left'  : el + ew + 10, 'top' : et };
        leftArrow.css({ left: '-9px' });
        tour.tooltip.prepend(leftArrow);
        break;

      case 'RB' :
        position = { 'left'  : el + ew + 10, 'top' : et + eh - th };
        leftArrow.css({ left: '-9px' });
        tour.tooltip.prepend(leftArrow);
        break;

      case 'LT' :
        position = { 'left'  : (el - tw) - 10, 'top' : et };
        rightArrow.css({ right: '-9px' });
        tour.tooltip.prepend(rightArrow);
        break;

      case 'LB' :
        position = { 'left'  : (el - tw) - 10, 'top' : et + eh - th};
        rightArrow.css({ right: '-9px' });
        tour.tooltip.prepend(rightArrow);
        break;

      case 'B'  :
        position = { 'left'  : el + ew/2 - tw/2, 'top' : (et + eh) + 10 };
        upArrow.css({ top: '-9px', left: '48%' });
        tour.tooltip.prepend(upArrow);
        break;

      case 'L'  :
        position = { 'left'  : (el - tw) - 17, 'top' : et + eh/2 - th/2 };
        rightArrow.css({ top: '40%', right: '-9px' });
        tour.tooltip.prepend(rightArrow);
        break;

      case 'T'  :
        position = { 'left'  : el + ew/2 - tw/2, 'top' : (et - th) - 10 };
        downArrow.css({ top: th, left: '48%' });
        tour.tooltip.append(downArrow);
        break;

      case 'R'  :
        position = { 'left'  : (el + ew) + 10, 'top' : et + eh/2 - th/2 };
        leftArrow.css({ top: '40%', left: '-9px' });
        tour.tooltip.prepend(leftArrow);
        break;

      case 'C'  :
        position = { 'left'  : el + ew/2 - tw/2, 'top' : et + eh/2 - th/2 };
    }
    return position;
  }

}


// Gauge for widgets on dashboard
// Copied from https://gist.github.com/tomerd/1499279

function Gauge(placeholderName, configuration)
{
  this.placeholderName = placeholderName;

  var self = this; // for internal d3 functions

  this.configure = function(configuration)
  {
    this.config = configuration;

    this.config.size = this.config.size * 0.9;

    this.config.raduis = this.config.size * 0.97 / 2;
    this.config.cx = this.config.size / 2;
    this.config.cy = this.config.size / 2;

    this.config.min = undefined != configuration.min ? configuration.min : 0;
    this.config.max = undefined != configuration.max ? configuration.max : 100;
    this.config.range = this.config.max - this.config.min;

    this.config.majorTicks = configuration.majorTicks || 5;
    this.config.minorTicks = configuration.minorTicks || 2;

    this.config.greenColor  = configuration.greenColor || "#109618";
    this.config.yellowColor = configuration.yellowColor || "#FF9900";
    this.config.redColor  = configuration.redColor || "#DC3912";

    this.config.transitionDuration = configuration.transitionDuration || 500;
  }

  this.render = function()
  {
    this.body = d3.select("#" + this.placeholderName)
              .append("svg:svg")
              .attr("class", "gauge")
              .attr("width", this.config.size)
              .attr("height", this.config.size);

    this.body.append("svg:circle")
          .attr("cx", this.config.cx)
          .attr("cy", this.config.cy)
          .attr("r", this.config.raduis)
          .style("fill", "#ccc")
          .style("stroke", "#000")
          .style("stroke-width", "0.5px");

    this.body.append("svg:circle")
          .attr("cx", this.config.cx)
          .attr("cy", this.config.cy)
          .attr("r", 0.9 * this.config.raduis)
          .style("fill", "#fff")
          .style("stroke", "#e0e0e0")
          .style("stroke-width", "2px");

    for (var index in this.config.greenZones)
    {
      this.drawBand(this.config.greenZones[index].from, this.config.greenZones[index].to, self.config.greenColor);
    }

    for (var index in this.config.yellowZones)
    {
      this.drawBand(this.config.yellowZones[index].from, this.config.yellowZones[index].to, self.config.yellowColor);
    }

    for (var index in this.config.redZones)
    {
      this.drawBand(this.config.redZones[index].from, this.config.redZones[index].to, self.config.redColor);
    }

    if (undefined != this.config.label)
    {
      var fontSize = Math.round(this.config.size / 9);
      this.body.append("svg:text")
            .attr("x", this.config.cx)
            .attr("y", this.config.cy / 2 + fontSize / 2)
            .attr("dy", fontSize / 2)
            .attr("text-anchor", "middle")
            .text(this.config.label)
            .style("font-size", fontSize + "px")
            .style("fill", "#333")
            .style("stroke-width", "0px");
    }

    var fontSize = Math.round(this.config.size / 16);
    var majorDelta = this.config.range / (this.config.majorTicks - 1);
    for (var major = this.config.min; major <= this.config.max; major += majorDelta)
    {
      var minorDelta = majorDelta / this.config.minorTicks;
      for (var minor = major + minorDelta; minor < Math.min(major + majorDelta, this.config.max); minor += minorDelta)
      {
        var point1 = this.valueToPoint(minor, 0.75);
        var point2 = this.valueToPoint(minor, 0.85);

        this.body.append("svg:line")
              .attr("x1", point1.x)
              .attr("y1", point1.y)
              .attr("x2", point2.x)
              .attr("y2", point2.y)
              .style("stroke", "#666")
              .style("stroke-width", "1px");
      }

      var point1 = this.valueToPoint(major, 0.7);
      var point2 = this.valueToPoint(major, 0.85);

      this.body.append("svg:line")
            .attr("x1", point1.x)
            .attr("y1", point1.y)
            .attr("x2", point2.x)
            .attr("y2", point2.y)
            .style("stroke", "#333")
            .style("stroke-width", "2px");

      if (major == this.config.min || major == this.config.max)
      {
        var point = this.valueToPoint(major, 0.63);

        this.body.append("svg:text")
              .attr("x", point.x)
              .attr("y", point.y)
              .attr("dy", fontSize / 3)
              .attr("text-anchor", major == this.config.min ? "start" : "end")
              .text(major)
              .style("font-size", fontSize + "px")
              .style("fill", "#333")
              .style("stroke-width", "0px");
      }
    }

    var pointerContainer = this.body.append("svg:g").attr("class", "pointerContainer");

    var midValue = (this.config.min + this.config.max) / 2;

    var pointerPath = this.buildPointerPath(midValue);

    var pointerLine = d3.svg.line()
                  .x(function(d) { return d.x })
                  .y(function(d) { return d.y })
                  .interpolate("basis");

    pointerContainer.selectAll("path")
              .data([pointerPath])
              .enter()
                .append("svg:path")
                  .attr("d", pointerLine)
                  .style("fill", "#dc3912")
                  .style("stroke", "#c63310")
                  .style("fill-opacity", 0.7)

    pointerContainer.append("svg:circle")
              .attr("cx", this.config.cx)
              .attr("cy", this.config.cy)
              .attr("r", 0.12 * this.config.raduis)
              .style("fill", "#4684EE")
              .style("stroke", "#666")
              .style("opacity", 1);

    var fontSize = Math.round(this.config.size / 10);
    pointerContainer.selectAll("text")
              .data([midValue])
              .enter()
                .append("svg:text")
                  .attr("x", this.config.cx)
                  .attr("y", this.config.size - this.config.cy / 4 - fontSize)
                  .attr("dy", fontSize / 2)
                  .attr("text-anchor", "middle")
                  .style("font-size", fontSize + "px")
                  .style("fill", "#000")
                  .style("stroke-width", "0px");

    this.redraw(this.config.value, 0);
  }

  this.buildPointerPath = function(value)
  {
    var delta = this.config.range / 13;

    var head = valueToPoint(value, 0.85);
    var head1 = valueToPoint(value - delta, 0.12);
    var head2 = valueToPoint(value + delta, 0.12);

    var tailValue = value - (this.config.range * (1/(270/360)) / 2);
    var tail = valueToPoint(tailValue, 0.28);
    var tail1 = valueToPoint(tailValue - delta, 0.12);
    var tail2 = valueToPoint(tailValue + delta, 0.12);

    return [head, head1, tail2, tail, tail1, head2, head];

    function valueToPoint(value, factor)
    {
      var point = self.valueToPoint(value, factor);
      point.x -= self.config.cx;
      point.y -= self.config.cy;
      return point;
    }
  }

  this.drawBand = function(start, end, color)
  {
    if (0 >= end - start) return;

    this.body.append("svg:path")
          .style("fill", color)
          .attr("d", d3.svg.arc()
            .startAngle(this.valueToRadians(start))
            .endAngle(this.valueToRadians(end))
            .innerRadius(0.65 * this.config.raduis)
            .outerRadius(0.85 * this.config.raduis))
          .attr("transform", function() { return "translate(" + self.config.cx + ", " + self.config.cy + ") rotate(270)" });
  }

  this.redraw = function(value, transitionDuration)
  {
    var pointerContainer = this.body.select(".pointerContainer");

    pointerContainer.selectAll("text").text(Math.round(value));

    var pointer = pointerContainer.selectAll("path");
    pointer.transition()
          .duration(undefined != transitionDuration ? transitionDuration : this.config.transitionDuration)
          //.delay(0)
          //.ease("linear")
          //.attr("transform", function(d)
          .attrTween("transform", function()
          {
            var pointerValue = value;
            if (value > self.config.max) pointerValue = self.config.max + 0.02*self.config.range;
            else if (value < self.config.min) pointerValue = self.config.min - 0.02*self.config.range;
            var targetRotation = (self.valueToDegrees(pointerValue) - 90);
            var currentRotation = self._currentRotation || targetRotation;
            self._currentRotation = targetRotation;

            return function(step)
            {
              var rotation = currentRotation + (targetRotation-currentRotation)*step;
              return "translate(" + self.config.cx + ", " + self.config.cy + ") rotate(" + rotation + ")";
            }
          });
  }

  this.valueToDegrees = function(value)
  {
    // thanks @closealert
    //return value / this.config.range * 270 - 45;
    return value / this.config.range * 270 - (this.config.min / this.config.range * 270 + 45);
  }

  this.valueToRadians = function(value)
  {
    return this.valueToDegrees(value) * Math.PI / 180;
  }

  this.valueToPoint = function(value, factor)
  {
    return {  x: this.config.cx - this.config.raduis * factor * Math.cos(this.valueToRadians(value)),
          y: this.config.cy - this.config.raduis * factor * Math.sin(this.valueToRadians(value))    };
  }

  // initialization
  this.configure(configuration);
}
