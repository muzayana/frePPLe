
// Django sets this variable in the admin/base.html template.
window.__admin_media_prefix__ = "/static/admin/";


// Adjust the breadcrumbs such that it fits on a single line.
// This function is called when the window is resized.
function breadcrumbs_reflow()
{
  var crumbs = $("#breadcrumbs");
  var height_one_line = Math.ceil($("#cockpitcrumb").height());
  // Show all elements previously hidden
  crumbs.children("span:hidden").show();
  // Hide the first crumbs till it all fits on a single line.
  crumbs.children("span").each(function() {
    if (crumbs.height() > height_one_line) $(this).hide();
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
    if ($('#undo').hasClass("ui-state-disabled")) return;
    $("#grid").trigger("reloadGrid");
    $('#save').addClass("ui-state-disabled").removeClass("bold");
    $('#undo').addClass("ui-state-disabled").removeClass("bold");
    $('#filter').removeClass("ui-state-disabled");
    $(window).off('beforeunload', upload.warnUnsavedChanges);
  },

  select : function ()
  {
    $('#filter').addClass("ui-state-disabled");
    $.jgrid.hideModal("#searchmodfbox_grid");
    $('#save').removeClass("ui-state-disabled").addClass("bold");
    $('#undo').removeClass("ui-state-disabled").addClass("bold");
    $(window).off('beforeunload', upload.warnUnsavedChanges);
    $(window).on('beforeunload', upload.warnUnsavedChanges);
  },

  save : function()
  {
    if ($('#save').hasClass("ui-state-disabled")) return;

    // Pick up all changed cells. If a function "getData" is defined on the
    // page we use that, otherwise we use the standard functionality of jqgrid.
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
            $('#popup').html(result.responseText)
              .dialog({
                title: gettext("Error saving data"),
                autoOpen: true,
                resizable: false,
                width: 'auto',
                height: 'auto'
              });
            $('#timebuckets').dialog('close');
            $.jgrid.hideModal("#searchmodfbox_grid");
            }
        });
  },

  validateSort: function(event)
  {
    if ($(this).attr('id') == 'grid_cb') return;
    if ($('#save').hasClass("ui-state-disabled"))
      jQuery("#grid").jqGrid('resetSelection');
    else
    {
      $('#popup').html("")
        .dialog({
          title: gettext("Save or cancel your changes first"),
          autoOpen: true,
          resizable: false,
          width: 'auto',
          height: 'auto',
          buttons: [
            {
              text: gettext("Save"),
              click: function() {
                upload.save();
                $('#popup').dialog('close');
                }
            },
            {
              text: gettext("Cancel"),
              click: function() {
                upload.undo();
                $('#popup').dialog('close');
                }
            }
            ]
        });
      event.stopPropagation();
    }
  }
}


//----------------------------------------------------------------------------
// Custom formatter functions for the grid cells. Most of the custom handlers
// just add an appropriate context menu.
//----------------------------------------------------------------------------

function linkunformat (cellvalue, options, cell) {
  return cellvalue;
}

jQuery.extend($.fn.fmatter, {
  percentage : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    return cellvalue + "%";
  },
  duration : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
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
  item : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='item'></span>";
  },
  customer : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='customer'></span>";
  },
  buffer : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='buffer'></span>";
  },
  resource : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='resource'></span>";
  },
  forecast : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='forecast'></span>";
  },
  demand : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='demand'></span>";
  },
  operation : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='operation'></span>";
  },
  calendar : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='calendar'></span>";
  },
  calendarbucket : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='calendarbucket'></span>";
  },
  location : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='location'></span>";
  },
  setupmatrix : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='setupmatrix'></span>";
  },
  user : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='user'></span>";
  },
  group : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='group'></span>";
  },
  flow : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='flow'></span>";
  },
  load : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='load'></span>";
  },
  bucket : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='bucket'></span>";
  },
  parameter : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='parameter'></span>";
  },
  skill : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='skill'></span>";
  },
  resourceskill : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='resourceskill'></span>";
  },
  project : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='project'></span>";
  },
  projectdeel : function(cellvalue, options, rowdata) {
    if (cellvalue === undefined || cellvalue ==='') return '';
    if (options['colModel']['popup']) return cellvalue;
    return cellvalue + "<span class='context fa fa-caret-right' role='projectdeel'></span>";
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
jQuery.extend($.fn.fmatter.item, {
    unformat : linkunformat
});
jQuery.extend($.fn.fmatter.buffer, {
    unformat : linkunformat
});
jQuery.extend($.fn.fmatter.resource, {
    unformat : linkunformat
});
jQuery.extend($.fn.fmatter.forecast, {
    unformat : linkunformat
});
jQuery.extend($.fn.fmatter.customer, {
    unformat : linkunformat
});
jQuery.extend($.fn.fmatter.operation, {
    unformat : linkunformat
});
jQuery.extend($.fn.fmatter.demand, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.location, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.calendar, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.setupmatrix, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.user, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.group, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.flow, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.load, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.bucket, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.parameter, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.skill, {
  unformat : linkunformat
});
jQuery.extend($.fn.fmatter.resourceskill, {
  unformat : linkunformat
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

   setSelectedRow: function(id)
   {
     if (grid.selected != undefined)
       $(this).jqGrid('setCell', grid.selected, 'select', null);
     grid.selected = id;
     $(this).jqGrid('setCell', id, 'select', '<button onClick="opener.dismissRelatedLookupPopup(window, grid.selected);" class="ui-button ui-button-text-only ui-widget ui-state-default ui-corner-all"><span class="ui-button-text" style="font-size:66%">'+gettext('Select')+'</span></button>');
   },

  // Renders the cross list in a pivot grid
  pivotcolumns : function  (cellvalue, options, rowdata)
  {
    var result = '';
    for (i in cross_idx)
    {
      if (result != '') result += '<br/>';
      result += cross[cross_idx[i]]['name'];
    }
    return result;
  },

  // Render the customization popup window
  showCustomize: function (pivot)
  {
    $.jgrid.hideModal("#searchmodfbox_grid");
    val = "<select id='configure' multiple='multiple' class='multiselect' name='fields' style='width:440px; height:200px; margin:10px; padding:10px'>" +
    "<optgroup label='Rows'>";
    var colModel = $("#grid")[0].p.colModel;
    var maxfrozen = 0;
    var skipped = 0;
    var graph = false;
    for (var i in colModel)
    {
      if (colModel[i].name == 'graph')
        graph = true;
      else if (colModel[i].name != "rn" && colModel[i].name != "cb" && colModel[i].counter != null && colModel[i].label != '' && !('alwayshidden' in colModel[i]))
      {
        if (colModel[i].frozen) maxfrozen = parseInt(i,10) + 1 - skipped;
        val += "<option value='" + (i) + "'";
        if (!colModel[i].hidden) val += " selected='selected'";
        if (colModel[i].key) val += " disabled='disabled'";
        val += ">" + colModel[i].label + "</option>";
      }
      else
        skipped++;
    }
    val += "</optgroup>";
    if (pivot)
    {
      // Add list of crosses
      val += "<optgroup label='Crosses'>";
      for (var j in cross_idx)
      {
        val += "<option value='" + (100+parseInt(cross_idx[j],10)) + "' selected='selected'";
        val += ">" + cross[cross_idx[j]]['name'] + "</option>";
      }
      for (var j in cross)
      {
        if (cross_idx.indexOf(parseInt(j,10)) > -1) continue;
        val += "<option value='" + (100+parseInt(j,10)) + "'";
        val += ">" + cross[j]['name'] + "</option>";
      }
      val += "</optgroup></select>";
    }
    else
    {
      // Add selection of number of frozen columns
      val += "</select>Frozen columns <select id='frozen'>";
      for (var i = 0; i <= 4; i++)
        if (i == maxfrozen)
          val += "<option value'" + i + "' selected='selected'>" + i + "</option>";
        else
          val += "<option value'" + i + "'>" + i + "</option>";
      val += "</select>";
    }
    $('#popup').html(val).dialog({
       title: gettext("Customize"),
       width: 465,
       height: 'auto',
       autoOpen: true,
       resizable: false,
       buttons: [{
         text: gettext("OK"),
         click: function() {
           var colModel = $("#grid")[0].p.colModel;
           var perm = [];
           var hiddenrows = [];
           if (colModel[0].name == "cb") perm.push(0);
           cross_idx = [];
           if (!graph)
             $("#grid").jqGrid('destroyFrozenColumns');
           $('#configure option').each(function() {
             val = parseInt(this.value,10);
             if (val < 100)
             {
               if (this.selected)
               {
                 $("#grid").jqGrid("showCol", colModel[val].name);
                 perm.push(val);
               }
               else
               {
                 hiddenrows.push(val);
                 if (pivot)
                   $("#grid").jqGrid('setColProp', colModel[val].name, {frozen:false});
                 $("#grid").jqGrid("hideCol", colModel[val].name);
               }
             }
             else if (this.selected)
               cross_idx.push(val-100);
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
             numfrozen = parseInt($("#frozen :selected").text())
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
           $(this).dialog("close");
         }},
         {
         text: gettext("Cancel"),
         click: function() { $(this).dialog("close"); }
         }]
       });
    $("#configure").multiselect({
      collapsableGroups: false,
      sortable: true,
      showEmptyGroups: true,
      locale: $("html")[0].lang,
      searchField: false
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
    result[reportkey] = {
      "rows": colArray,
      "page": page,
      "filter": $('#grid').getGridParam("postData").filters
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
    $.ajax({
      url: '/settings/',
      type: 'POST',
      contentType: 'application/json; charset=utf-8',
      data: JSON.stringify(result),
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
  },

  //This function is called when a cell is just being selected in an editable
  //grid. It is used to either a) select the content of the cell (to make
  //editing it easier) or b) display a date picker it the field is of type
  //date.
  afterEditCell: function (rowid, cellname, value, iRow, iCol)
  {
   var cell = document.getElementById(iRow+'_'+cellname);
   var colmodel = jQuery("#grid").jqGrid ('getGridParam', 'colModel')[iCol];
   if (colmodel.formatter == 'date')
   {
     if (colmodel.formatoptions['srcformat'] == "Y-m-d")
       $(cell).datepicker({
         showOtherMonths: true, selectOtherMonths: true,
         dateFormat: "yy-mm-dd", changeMonth:true,
         changeYear:true, yearRange: "c-1:c+5"
         });
     else
       $(cell).datepicker({
         showOtherMonths: true, selectOtherMonths: true,
         dateFormat: "yy-mm-dd 00:00:00", changeMonth:true,
         changeYear:true, yearRange: "c-1:c+5"
         });
   }
   else
     $(cell).select();
  },

  // Display dialog for exporting CSV-files
  showExport: function(only_list)
  {
    // The argument is true when we show a "list" report.
    // It is false for "table" reports.
    if (only_list)
      $('#popup').html(
        gettext("Export format") + '&nbsp;&nbsp;:&nbsp;&nbsp;<select name="csvformat" id="csvformat">' +
        '<option value="spreadsheetlist" selected="selected">' + gettext("Spreadsheet list") + '</option>' +
        '<option value="csvlist">' + gettext("CSV list") +'</option></select>'
        );
    else
        $('#popup').html(
        gettext("Export format") + '&nbsp;&nbsp;:&nbsp;&nbsp;<select name="csvformat" id="csvformat">' +
        '<option value="spreadsheettable" selected="selected">' + gettext("Spreadsheet table") + '</option>' +
        '<option value="spreadsheetlist">' + gettext("Spreadsheet list") + '</option>' +
        '<option value="csvtable">' + gettext("CSV table") +'</option>'+
        '<option value="csvlist">' + gettext("CSV list") +'</option></select>'
        );
    $('#popup').dialog({
        title: gettext("Export CSV or Excel file"),
        autoOpen: true, resizable: false, width: 390, height: 'auto',
        buttons: [
          {
            text: gettext("Export"),
            click: function() {
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
              $('#popup').dialog().dialog('close');
            }
          },
          {
            text: gettext("Cancel"),
            click: function() { $(this).dialog("close"); }
          }
          ]
        });
    $('#timebuckets').dialog().dialog('close');
    $.jgrid.hideModal("#searchmodfbox_grid");
  },

  // Display time bucket selection dialog
  showBucket: function()
  {
    // Show popup
    $('#popup').dialog().dialog('close');
    $.jgrid.hideModal("#searchmodfbox_grid");
    $( "#horizonstart" ).datepicker({
        showOtherMonths: true, selectOtherMonths: true,
        changeMonth:true, changeYear:true, yearRange: "c-1:c+5", dateFormat: 'yy-mm-dd'
      });
    $( "#horizonend" ).datepicker({
        showOtherMonths: true, selectOtherMonths: true,
        changeMonth:true, changeYear:true, yearRange: "c-1:c+5", dateFormat: 'yy-mm-dd'
      });
    $('#timebuckets').dialog({
       autoOpen: true, resizable: false, width: 390,
       buttons: [
         {
           text: gettext("OK"),
           click: function() {
            // Compare old and new parameters
            var params = $('#horizonbuckets').val() + '|' +
              $('#horizonstart').val() + '|' +
              $('#horizonend').val() + '|' +
              ($('#horizontype').is(':checked') ? "True" : "False") + '|' +
              $('#horizonlength').val() + '|' +
              $('#horizonunit').val();
            if (params == $('#horizonoriginal').val())
              // No changes to the settings. Close the popup.
              $(this).dialog('close');
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
            }
           }
         },
         {
           text: gettext("Cancel"),
           click: function() { $(this).dialog("close"); }
         }
         ]
      });
  },

  //Display dialog for copying or deleting records
  showDelete : function()
  {
    if ($('#delete_selected').hasClass("ui-state-disabled")) return;
    var sel = jQuery("#grid").jqGrid('getGridParam','selarrrow');
    if (sel.length == 1)
    {
      // Redirect to a page for deleting a single entity
      location.href = location.pathname + encodeURI(sel[0]) + '/delete/';
    }
    else if (sel.length > 0)
    {
     $('#popup').html(
       interpolate(gettext('You are about to delete %s objects AND ALL RELATED RECORDS!'), [sel.length], false)
       ).dialog({
         title: gettext("Delete data"),
         autoOpen: true,
         resizable: false,
         width: 'auto',
         height: 'auto',
         buttons: [
           {
             text: gettext("Confirm"),
             click: function() {
               $.ajax({
                 url: location.pathname,
                 data: JSON.stringify([{'delete': sel}]),
                 type: "POST",
                 contentType: "application/json",
                 success: function () {
                   $("#delete_selected").addClass("ui-state-disabled").removeClass("bold");
                   $("#copy_selected").addClass("ui-state-disabled").removeClass("bold");
                   $('.cbox').prop("checked", false);
                   $('#cb_grid.cbox').prop("checked", false);
                   $("#grid").trigger("reloadGrid");
                   $('#popup').dialog('close');
                   },
                 error: function (result, stat, errorThrown) {
                   $('#popup').html(result.responseText)
                     .dialog({
                       title: gettext("Error deleting data"),
                       autoOpen: true,
                       resizable: true,
                       width: 'auto',
                       height: 'auto'
                     });
                   $('#timebuckets').dialog('close');
                   $.jgrid.hideModal("#searchmodfbox_grid");
                   }
               });
             }
           },
           {
             text: gettext("Cancel"),
             click: function() { $(this).dialog("close"); }
           }
           ]
       });
     $('#timebuckets').dialog().dialog('close');
     $.jgrid.hideModal("#searchmodfbox_grid");
   }
  },

  showCopy: function()
  {
   if ($('#copy_selected').hasClass("ui-state-disabled")) return;
   var sel = jQuery("#grid").jqGrid('getGridParam','selarrrow');
   if (sel.length > 0)
   {
     $('#popup').html(
       interpolate(gettext('You are about to duplicate %s objects'), [sel.length], false)
       ).dialog({
         title: gettext("Copy data"),
         autoOpen: true,
         resizable: false,
         width: 'auto',
         height: 'auto',
         buttons: [
           {
             text: gettext("Confirm"),
             click: function() {
               $.ajax({
                 url: location.pathname,
                 data: JSON.stringify([{'copy': sel}]),
                 type: "POST",
                 contentType: "application/json",
                 success: function () {
                   $("#delete_selected").addClass("ui-state-disabled").removeClass("bold");
                   $("#copy_selected").addClass("ui-state-disabled").removeClass("bold");
                   $('.cbox').prop("checked", false);
                   $('#cb_grid.cbox').prop("checked", false);
                   $("#grid").trigger("reloadGrid");
                   $('#popup').dialog().dialog('close');
                   },
                 error: function (result, stat, errorThrown) {
                   $('#popup').html(result.responseText)
                     .dialog({
                       title: gettext("Error copying data"),
                       autoOpen: true,
                       resizable: true,
                       width: 'auto',
                       height: 'auto'
                     });
                   $('#timebuckets').dialog().dialog('close');
                   $.jgrid.hideModal("#searchmodfbox_grid");
                   }
               });
             }
           },
           {
             text: gettext("Cancel"),
             click: function() { $(this).dialog("close"); }
           }
           ]
       });
     $('#timebuckets').dialog().dialog('close');
     $.jgrid.hideModal("#searchmodfbox_grid");
   }
  },

  // Display filter dialog
  showFilter: function()
  {
    if ($('#filter').hasClass("ui-state-disabled")) return;
    $('#timebuckets,#popup').dialog().dialog('close');
    jQuery("#grid").jqGrid('searchGrid', {
      closeOnEscape: true,
      multipleSearch:true,
      multipleGroup:true,
      overlay: 0,
      sopt: ['eq','ne','lt','le','gt','ge','bw','bn','in','ni','ew','en','cn','nc'],
      onSearch : function() {
        grid.saveColumnConfiguration();
        var s = jQuery("#fbox_grid").jqFilter('toSQLString');
        if (s) $('#curfilter').html(gettext("Filtered where") + " " + s);
        else $('#curfilter').html("");
        },
      onReset : function() {
        if (initialfilter != '') $('#curfilter').html(gettext("Filtered where") + " " + jQuery("#fbox_grid").jqFilter('toSQLString'));
        else $('#curfilter').html("");
        }
      });
  },

  markSelectedRow: function(id)
  {
    var sel = jQuery("#grid").jqGrid('getGridParam','selarrrow').length;
    if (sel > 0)
    {
      $("#copy_selected").removeClass("ui-state-disabled").addClass("bold");
      $("#delete_selected").removeClass("ui-state-disabled").addClass("bold");
    }
    else
    {
      $("#copy_selected").addClass("ui-state-disabled").removeClass("bold");
      $("#delete_selected").addClass("ui-state-disabled").removeClass("bold");
    }
  },

  markAllRows: function()
  {
    if ($(this).is(':checked'))
    {
      $("#copy_selected").removeClass("ui-state-disabled").addClass("bold");
      $("#delete_selected").removeClass("ui-state-disabled").addClass("bold");
      $('.cbox').prop("checked", true);
    }
    else
    {
      $("#copy_selected").addClass("ui-state-disabled").removeClass("bold");
      $("#delete_selected").addClass("ui-state-disabled").removeClass("bold");
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
// Code for sending dashboard configuration to the server.
//----------------------------------------------------------------------------

var dashboard = {
  save : function(extra)
  {
    // Loop over all columns
    var columns = [];
    var width = 0;
    $(".column").each(function() {
       var widgets = [];
       // Loop over all widgets
       $(this).find(".portlet-config").each(function() {
         widgets.push( [$(this).attr("data-name"), {}] );
       });
       var x = $(this).css('width');
       x = parseInt(x.substring(0, x.length - 2));
       width += x;
       columns.push({'width': x, 'widgets': widgets});
    });
    // Convert column width to a percentage
    for (var i = 0; i < columns.length; i++)
      columns[i]['width'] = Math.round(columns[i]['width'] * 100.0 / width) + "%";
    // Adding an extra widget
    if ($.type(extra) === "string")
      columns[columns.length-1]['widgets'].push( [extra,{}] );
    // Send the results to the server
    $.ajax({
      url: '/settings/',
      type: 'POST',
      contentType: 'application/json; charset=utf-8',
      data: JSON.stringify({"freppledb.common.cockpit": columns}),
      success: function() {
        // Reload page if a widget was added
        if ($.type(extra) === "string")
          window.location.href = window.location.href;
        },
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
  },

  customize: function()
  {
    var txt = '<table style="text-align:center">' +
      '<tr><td><label for="layout1"><div class="customlayout" style="width:50px"></div></label><br/>' +
      '<input id="layout1" type="radio" name="layout" value="100"></td>' +
      '<td><label for="layout2"><div class="customlayout" style="width:25px"></div><div class="customlayout" style="width:25px"></div></label><br/>' +
      '<input id="layout2" type="radio" name="layout" value="50,50"></td>' +
      '<td><label for="layout3"><div class="customlayout" style="width:33px"></div><div class="customlayout" style="width:17px"></div></label><br/>' +
      '<input id="layout3" type="radio" name="layout" value="66,33"></td></tr>' +
      '<tr><td><label for="layout4"><div class="customlayout" style="width:17px"></div><div class="customlayout" style="width:16px"></div><div class="customlayout" style="width:17px"></div></label><br/>' +
      '<input id="layout4" type="radio" name="layout" value="34,33,33"></td>' +
      '<td><label for="layout5"><div class="customlayout" style="width:20px"></div><div class="customlayout" style="width:15px"></div><div class="customlayout" style="width:15px"></div></label><br/>' +
      '<input id="layout5" type="radio" name="layout" value="40,30,30"></td>' +
      '<td><label for="layout6"><div class="customlayout" style="width:25px"></div><div class="customlayout" style="width:12px"></div><div class="customlayout" style="width:13px"></div></label><br/>' +
      '<input id="layout6" type="radio" name="layout" value="50,25,25"></td></tr>' +
      '</table><br/>' +
      gettext('Add') + '&nbsp;&nbsp;<select name="add"><option value=""></option>';
    for (i in hiddenwidgets)
      txt += '<option value="' + hiddenwidgets[i][0] + '">' + hiddenwidgets[i][1] + "</option>";
    txt += '</select>';
    $('#popup')
      .html(txt)
      .dialog({
        title: gettext("Customize"),
        width: 'auto',
        height: 'auto',
        autoOpen: true,
        resizable: false,
        buttons: [
            {
              text: gettext("OK"),
              click: function() {
                var sel = $('#popup input[name=layout]:checked').val();
                if (sel !== undefined)
                {
                  // A layout was selected
                  sel = sel.split(",");
                  var lastcol;
                  var count = -1;
                  $(".column").each(function(idx) {
                    if (idx >= sel.length)
                    {
                      // Remove column
                      $(this).find(".portlet").each(function() {$(lastcol).append($(this));});
                      $(this).remove();
                    }
                    else
                    {
                      // Resize column
                      $(this).css('width',sel[idx] + '%');
                      lastcol = this;
                    }
                    count = idx;
                    });
                  while (count < sel.length-1)
                  {
                    // Add column
                    count += 1;
                    $(lastcol).after('<div class="column ui-sortable" style="width:' + sel[count] + '%"></div>');
                    $(".column").sortable({
                      connectWith: ".column",
                      handle: ".portlet-header",
                      cancel: ".portlet-toggle",
                      placeholder: "portlet-placeholder ui-corner-all",
                      stop: dashboard.save
                    });
                  }
                }
                var sel = $('#popup option:selected').val();
                if (sel != '')
                  dashboard.save(sel);
                else
                  dashboard.save();
                $(this).dialog("close");
              }
            },
            {
              text: gettext("Reset"),
              click: function() {
                $.ajax({
                  url: '/settings/',
                  type: 'POST',
                  contentType: 'application/json; charset=utf-8',
                  data: JSON.stringify({"freppledb.common.cockpit": ""}),
                  success: function() {
                    // Reload page
                    window.location.href = window.location.href;
                     },
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
                $(this).dialog("close");
                }
            },
            {
              text: gettext("Cancel"),
              click: function() { $(this).dialog("close"); }
            }
            ]
        });
  }
}

//----------------------------------------------------------------------------
// Code for customized autocomplete widget.
// The customization creates unselectable categories and selectable list
// items.
//----------------------------------------------------------------------------

$.widget( "custom.catcomplete", $.ui.autocomplete, {
  _renderItem: function( ul, item) {
    if (item.value == undefined)
      return $( "<li class='ui-autocomplete-category'>" + item.label + "</li>" ).appendTo( ul );
    else
      return $( "<li></li>" )
      .data( "item.autocomplete", item )
      .append( $( "<a></a>" ).text( item.value ) )
      .appendTo( ul );
  },



});


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
  var database = $('#database').val();
  database = (database===undefined || database==='default') ? '' : '/' + database;
  $("#search").catcomplete({
    source: database + "/search/",
    minLength: 2,
    select: function( event, ui ) {
      window.location.href = database + "/data/" + encodeURIComponent(ui.item.app)
         + '/' + encodeURIComponent(ui.item.label) + "/" + admin_escape(ui.item.value) + "/";
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
  $('#popup').html(
    '<form id="uploadform" enctype="multipart/form-data" method="post" action="'
    + (typeof(url) != 'undefined' ? url : '') + '">' +
    '<input type="hidden" name="csrfmiddlewaretoken" value="' + getToken() + '"/>' +
    gettext('Load an Excel file or a CSV-formatted text file.') + '<br/>' +
    gettext('The first row should contain the field names.') + '<br/><br/>' +
    '<input type="checkbox" name="erase" value="yes"/>&nbsp;&nbsp;' + gettext('First delete all existing records AND ALL RELATED TABLES') + '<br/><br/>' +
    gettext('Data file') + ':<input type="file" id="csv_file" name="csv_file"/></form>'
    ).dialog({
      title: gettext("Import CSV or Excel file"),
      autoOpen: true, resizable: false, width: 390, height: 'auto',
      buttons: [
        {
          text: gettext("Import"),
          click: function() { if ($("#csv_file").val() != "") $("#uploadform").submit(); }
        },
        {
          text: gettext("Cancel"),
          click: function() { $(this).dialog("close"); }
        }
        ]
    });
  $('#timebuckets').dialog().dialog('close');
  $.jgrid.hideModal("#searchmodfbox_grid");
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
  var el = $('#database');
  var db = el.val();
  var cur = el.attr('name');
  // Change the location
  if (cur == db)
    return;
  else if (cur == 'default')
    window.location.href = window.location.href.replace(window.location.pathname, "/"+db+window.location.pathname);
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
    var bucketwidth = el.width() / numbuckets;
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
        .attr("class", "ui-tooltip ui-widget ui-corner-all ui-widget-content")
        .style("position", "absolute");
    // Update content and display
    tt.html('<div class="ui-tooltip-content">' + txt + '</div>')
      .style('left', (d3.event.pageX + 5) + "px")
      .style('top', (d3.event.pageY - 28) + "px")
      .style('display', 'block');
    /*
    // Compute position
    var limit = $(window).width() - $("#tooltip").width() - 15;
    var x = d3.event.pageX + 5;
    if (x > limit) x = limit;
    var y = d3.event.pageY - 28;
    limit = $(window).height() - $("#tooltip").height() - 15;
    if (y > limit) y = limit;
    tt.style('left', x + "px")
      .style('top', y + "px");
    */
    d3.event.stopPropagation();
  },

  hideTooltip: function()
  {
    d3.select("#tooltip").style('display', 'none');
    d3.event.stopPropagation();
  },

  moveTooltip: function()
  {
    /*
    var limit = $(window).width() - $("#tooltip").width() - 15;
    var x = d3.event.pageX + 5;
    if (x > limit) x = limit;
    var y = d3.event.pageY - 28;
    limit = $(window).height() - $("#tooltip").height() - 15;
    if (y > limit) y = limit;
    */
    d3.select("#tooltip")
      .style('left', (d3.event.pageX + 5) + "px")
      .style('top', (d3.event.pageY - 28) + "px");
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
        result.push('<text class="svgheadertext" x="' + Math.floor((x1+x2)/2) + '" y="31">' + $.datepicker.formatDate("M", bucketstart) + '</text>');
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
        result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling*3.5) + '" y="31">' + $.datepicker.formatDate("mm-dd", bucketstart) + '</text>');
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
        result.push('<text class="svgheadertext" x="' + Math.floor((x1+x2)/2) + '" y="13">' + $.datepicker.formatDate("M yy", bucketstart) + '</text>');
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
        result.push('<text class="svgheadertext" x="' + (x + scaling*7.0/2.0) + '" y="31">' + $.datepicker.formatDate("yy-mm-dd", bucketstart) + '</text>');
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
        result.push('<text class="svgheadertext" x="' + Math.floor((x1+x2)/2) + '" y="13">' + $.datepicker.formatDate("M yy", bucketstart) + '</text>');
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
          result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling*7/2) + '" y="13">' + $.datepicker.formatDate("yy-mm-dd", bucketstart) + '</text>');
        }
        else
        {
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="17" x2="' + Math.floor(x) + '" y2="34"/>');
        }
        result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling/2) + '" y="31">' + $.datepicker.formatDate("d", bucketstart) + '</text>');
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
          result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling*7/2) + '" y="13">' + $.datepicker.formatDate("yy-mm-dd", bucketstart) + '</text>');
        }
        else
        {
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="17" x2="' + Math.floor(x) + '" y2="34"/>');
        }
        result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling/2) + '" y="31">' + $.datepicker.formatDate("dd M", bucketstart) + '</text>');
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
          result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling*3.5) + '" y="13">' + $.datepicker.formatDate("yy-mm-dd", bucketstart) + '</text>');
        }
        else
          result.push('<line class="time" x1="' + Math.floor(x) + '" y1="17" x2="' + Math.floor(x) + '" y2="34"/>');
        result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling/2) + '" y="31">' + $.datepicker.formatDate("D dd M", bucketstart) + '</text>');
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
          result.push('<text class="svgheadertext" x="' + Math.floor(x + scaling/2) + '" y="13">' + $.datepicker.formatDate("D yy-mm-dd", bucketstart) + '</text>');
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
    var viewstart = new Date(horizonstart.getTime());
    var viewend = new Date(horizonend.getTime());
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
     $("body").append( '<div id="tour" style="padding-bottom:20px; display:none">' +
         tourdata[tour.chapter]['description']  + '<br/><br/><br/></div>');
     $("#tour").dialog({
      title: gettext("Guided tour"),
      autoOpen: true,
      resizable: false,
      width: 390,
      height: 'auto',
      position: "right bottom",
      modal: false,
      dialogClass: "tourguide",
      close: function() {
          $('#tour').remove();
          $('#tourtooltip').remove();
          tour.tooltip.css({ 'display' : 'none' }).html('');
          tour.chapter = 0;
          tour.step = 0;
          tour.autoplay = false;
          if (tour.timeout)
          {
            clearTimeout(tour.timeout);
            tour.timeout = null;
          }
        },
      buttons: [
        {
          id: "tourprevious",
          text: gettext("Previous"),
          icons: { primary: "ui-icon-seek-prev" },
          click: tour.prev
        },
        {
          text: (tour.autoplay != 0) ? gettext("Stop") : gettext("Play"),
          icons: { primary: (tour.autoplay != 0) ? "ui-icon-pause" : "ui-icon-play" },
          click: tour.toggleAutoplay
        },
        {
          id: "tournext",
          text: gettext("Next"),
          icons: { primary: "ui-icon-seek-next" },
          click: tour.next
        }
        ]
      });

     // Create the tooltip
     tour.tooltip = $('<div>',{id:'tourtooltip', class:'tourtooltip ui-dialog ui-widget ui-widget-content ui-corner-all ui-front', html:''}).css({
       'display': 'none', 'overflow': 'visible'
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
      $("#tourprevious").button("disable");
    else
      $("#tourprevious").button("enable");
    // Next button
    if ((tour.chapter >= tourdata.length-1) && (tour.step >= tourdata[tour.chapter]['steps'].length-1))
      $("#tournext").button("disable");
    else
      $("#tournext").button("enable");
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
      var icn = $(".ui-icon-pause");
      icn.toggleClass("ui-icon-pause ui-icon-play");
      icn.next().html(gettext("Play"));
      tour.autoplay = 0;
      clearTimeout(tour.timeout);
      tour.timeout = null;
    }
    else
    {
      var icn = $(".ui-icon-play");
      icn.toggleClass("ui-icon-play ui-icon-pause");
      icn.next().html(gettext("Stop"));
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
        position = { 'left'  : (el - tw) - 10, 'top' : et + eh/2 - th/2 };
        rightArrow.css({ right: '-9px' });
        tour.tooltip.prepend(rightArrow);
        break;

      case 'T'  :
        position = { 'left'  : el + ew/2 - tw/2, 'top' : (et - th) - 10 };
        downArrow.css({ top: th, left: '48%' });
        tour.tooltip.append(downArrow);
        break;

      case 'R'  :
        position = { 'left'  : (el + ew) + 10, 'top' : et + eh/2 - th/2 };
        leftArrow.css({ left: '-9px' });
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
