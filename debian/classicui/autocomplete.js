// Written by Kepi (kepi.cz)

$(document).ready(function() {
  'use strict';
  var min_char = 3;
  var max_items_per_cat = 10;
  var display_services = true;

  window.icingaAC = {
    // This is number of miliseconds to cache partial result
    // so we don't query server again and again for same terms
    expireMsecs: 60000,

    // internal variables
    shorten: undefined,
    results: {}
  }

  // extension for category support in autocomplete result
  $.widget("custom.catcomplete", $.ui.autocomplete, {
    _renderMenu: function( ul, items ) {
      var that = this;
      var currentCategory = "";
      var current_id = 0;
      $.each( items, function( index, item ) {
        if (item.category != currentCategory) {
          $('<li/>').addClass('ui-autocomplete-category').html(item.category).appendTo(ul);
          currentCategory = item.category;
          current_id = 0;
        }
	if (current_id < max_items_per_cat)
          that._renderItemData( ul, item );
	if (current_id == max_items_per_cat)
          $('<li/>').addClass('ui-menu-item').html("...").appendTo(ul);
	current_id++;
      });
    }
  });

  // extend renderItem so we can add custom class
  $.extend( $.ui.autocomplete.prototype, {
    _renderItem: function( ul, item ) {
    return $( '<li class="status' + item.status + '">' )
      .append( $( "<a>" ).text( item.label ) )
      .appendTo( ul );
    }
  });

  // autocomplete function
  $("#autocomplete").catcomplete( { minLength: min_char, delay : 700 }, {
    // select function is used to open url for selected item
    select: function( event, ui ) { top.frames['main'].location.href = ui.item.url },
    // get results from icinga API
    source: function(request, response) {
      var shorten = request.term.substring(0, min_char);
      var output = [];

      if (window.icingaAC.shorten === undefined || shorten != window.icingaAC.shorten || window.icingaAC.results[window.icingaAC.shorten].expire < (new Date).getTime()) {
        $.ajax({
	  async: false,
          url: '/cgi-bin/icinga2-classicui/status.cgi?livesearchdata',
          dataType: 'json',
          data: {
            search_string: shorten // parameter for API search
          },
          success: function(data) {

            // prepare hosts data
            if ( data.status.hosts ) {
              $.each( data.status.hosts, function(index, item) {
                output.push( {
                  category: 'Hosts',
                  url: '/cgi-bin/icinga2-classicui/status.cgi?search_string=' + encodeURIComponent(item.host_name),
                  label: item.host_display_name,
		  local_match: new Array( item.host_name, item.host_display_name, item.host_alias, item.host_address, item.host_address6 ),
                  status: item.status
                });
              });
            }

            // prepare services data
            if ( data.status.services && display_services) {
              $.each( data.status.services, function(index, item) {
                output.push( {
                  category: 'Services ' + item.host_name,
                  url: '/cgi-bin/icinga2-classicui/extinfo.cgi?type=2&host=' + encodeURIComponent(item.host_name) + '&service=' + encodeURIComponent(item.service_description),
                  label: item.service_display_name,
                  local_match: new Array( item.service_description, item.service_display_name,
                                          item.host_name + ' ' + item.service_description,
                                          item.host_name + ' ' + item.service_display_name,
                                          item.host_display_name + ' ' + item.service_description,
                                          item.host_display_name + ' ' + item.service_display_name ),
                  value: item.host_name + ' ' + item.service_description,
                  status: item.status
                });
              });
            }

            // prepare host groups data
            if ( data.status.hostgroups ) {
              $.each( data.status.hostgroups, function(index, item) {
                output.push( {
                  category: 'Hostgroups',
                  url: '/cgi-bin/icinga2-classicui/status.cgi?style=overview&hostgroup=' + encodeURIComponent(item.hostgroup_name),
                  local_match: new Array( item.hostgroup_name, item.hostgroup_alias ),
                  label: item.hostgroup_alias,
                  status: "group"
                });
              });
            }

            // prepare service groups data
            if ( data.status.servicegroups ) {
              $.each( data.status.servicegroups, function(index, item) {
                output.push( {
                  category: 'Servicegroups',
                  url: '/cgi-bin/icinga2-classicui/status.cgi?style=overview&servicegroup=' + encodeURIComponent(item.servicegroup_name),
                  local_match: new Array( item.servicegroup_name, item.servicegroup_alias ),
                  label: item.servicegroup_alias,
                  status: "group"
                });
              });
            }

            window.icingaAC.shorten = shorten;
            window.icingaAC.results[window.icingaAC.shorten] = {
              expire: (new Date).getTime() + window.icingaAC.expireMsecs,
              data: output
            }
          }
        })
      }

      var matcher = new RegExp( request.term.replace(/\*/g, ".*"), "i" );
      response( $.grep( window.icingaAC.results[window.icingaAC.shorten].data, function( item ){
        var match_result = false;
        $.each( item.local_match, function( index, match_item ) {
          if (match_item !== undefined && matcher.test( match_item )) {
            match_result = true;
            return;
          }
        });

        return match_result;
      }) );

    }
  }).bind('focus', function(){ $(this).catcomplete("search"); } );
});

// vim: syntax=javascript ts=2 sw=2 sts=2 sr et
