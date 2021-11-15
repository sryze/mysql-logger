var escape = document.createElement('textarea');
var queries = [];
var emptyMessage = document.getElementById('empty-message');
var tableContainer = document.getElementById('table-container');
var table = document.getElementById('table');
var infoPanel = document.getElementById('info-panel');

function escapeHTML(html) {
    escape.textContent = html;
    return escape.innerHTML;
}

function hasClass(element, className) {
  return new RegExp('\\b' + className + '\\b', 'g').test(element.className);
}

function addClass(element, className) {
  if (!hasClass(element, className)) {
    element.className += ' ' + className;
  }
}

function removeClass(element, className) {
  var classRegExp = new RegExp('\\s*\\b' + className + '\\b\\s*', 'g');
  element.className = element.className.replace(classRegExp, ' ');
}

function toggleClass(element, className) {
  if (hasClass(element, className)) {
    removeClass(element, className);
  } else {
    addClass(element, className);
  }
}

function getQueryStringParams() {
  if (!window.location.hash || window.location.hash.indexOf('#?') != 0) {
    return {};
  }
  var params = {};
  var queryString = window.location.hash.substring(2);
  var queryEntries = queryString.split('&');
  for (var i = 0; i < queryEntries.length; i++) {
    var param = queryEntries[i];
    var components = param.split('=');
    var key = components[0];
    var value = components.length >= 2 ? components[1] : null;
    params[key] = value;
  }
  return params;
}

function renderTemplate(html, data) {
  for (var key in data) {
    if (data.hasOwnProperty(key)) {
      html = html.replace(new RegExp('{{\\s*' + key + '\\s*}}', 'g'),
                          data[key] != null ? data[key] : '');
    }
  }
  return html;
}

function formatDateTime(date) {
  function pad(n) {
    return n < 10 ? '0' + n : n;
  }
  return date.getFullYear()
    + '-' + pad(date.getMonth() + 1)
    + '-' + pad(date.getDate())
    + ' ' + pad(date.getHours())
    + ':' + pad(date.getMinutes())
    + ':' + pad(date.getSeconds());
}

function appendCell(row, text) {
  var content = document.createElement('div');
  content.className = 'cell-content';
  content.appendChild(document.createTextNode(text));
  var row = row.insertCell();
  row.appendChild(content)
  return row;
}

function appendQueryRow(table, query) {
  var row = table.insertRow();
  appendCell(row, query.queryId);
  appendCell(row, query.database != null ? query.database : '');
  appendCell(row, query.user);
  appendCell(row, query.query).className = 'query-cell';
  appendCell(row, query.startTimeString);
  return row;
}

function deleteQueryRow(table, index) {
  table.deleteRow(index);
}

function updateInfoPanelForQuery(query) {
  infoPanel.innerHTML = createQueryInfo(query);
}

function createHighlightedQuery(queryText) {
  return '<span class="code">' + escapeHTML(queryText) + '</span>';
}

function createQueryInfo(query) {
  return '<b>Status:</b><br>'
    + query.status
    + '<br><br><b>Query:</b><br>'
    + createHighlightedQuery(query.query)
    + '<br><br><b>Query ID:</b><br>'
    + query.queryId
    + (
      query.database != null
        ? '<br><br><b>Database:</b><br>' + query.database
        : ''
    )
    + '<br><br><b>User:</b><br>'
    + query.user
    + (
      query.errorMessage != null
        ? '<br><br><b>Error Message:</b><br>' + query.errorMessage
        : ''
    )
    + '<br><br><b>Started:</b><br>'
    + query.startTimeString
    + (
      query.executionTime != null
        ? '<br><br><b>Time:</b><br>' + query.executionTime / 1000.0 + ' sec'
        : ''
    );
}

function onQueryStart(event, options) {
  options = options || {};

  if (queries.length == 0) {
    addClass(emptyMessage, 'hidden');
  }

  var maxQueryCount = options.maxQueryCount || Infinity;
  if (queries.length >= maxQueryCount) {
    queries.splice(1, 1);
    deleteQueryRow(table, 1);
  }

  removeClass(table, 'hidden');

  var scrollShouldFollow = tableContainer.scrollTop
      >= tableContainer.scrollHeight - tableContainer.clientHeight - 30;
  var query = {
    database: event.database,
    user: event.user,
    query: event.query,
    queryId: event.query_id,
    status: 'Executing',
    startTime: event.time,
    startTimeString:
      event.time ? formatDateTime(new Date(event.time)) : ''
  };
  queries.push(query);

  var row = appendQueryRow(table, query);
  row.addEventListener('click', function() {
    var isSelected = hasClass(row, 'selected');
    if (isSelected) {
      removeClass(row, 'selected');
      addClass(infoPanel, 'hidden');
    } else {
      for (var i = 0; i < table.rows.length; i++) {
        removeClass(table.rows[i], 'selected');
      }
      addClass(row, 'selected');
      removeClass(infoPanel, 'hidden');
      updateInfoPanelForQuery(query);
    }
    query.isSelected = !isSelected;
  });

  if (scrollShouldFollow) {
    tableContainer.scrollTop = tableContainer.scrollHeight;
  }
}

function onQueryEnd(event) {
  if (!event.query_id) {
    return;
  }

  var success = !event.error_code;
  for (var i = 0; i < queries.length; i++) {
    var query = queries[i];
    if (query.queryId == event.query_id) {
      query.executionTime = event.time - query.startTime;
      query.errorMessage = event.error_message;
      query.status = success ? 'Success' : 'Error';
      addClass(table.rows[i + 1], success ? 'success' : 'error');
      if (query.isSelected) {
        updateInfoPanelForQuery(query);
      }
      break;
    }
  }
}

window.addEventListener('DOMContentLoaded', function() {
  var params = getQueryStringParams();
  var host = params.host || window.location.hostname || 'localhost';
  var uiPort = window.location.port ? +window.location.port : 13306;
  var port = params.port || uiPort + 1;
  var url = 'ws://' + host + ':' + port;
  var socket = new WebSocket(url);

  socket.addEventListener('open', function(event) {
    console.log('WebSocket opened!');
  });

  socket.addEventListener('error', function(event) {
    alert('Could not connect to ' + url + '.');
  });

  socket.addEventListener('message', function(event) {
    console.log('WebSocket message:', event);

    var eventData = JSON.parse(event.data);
    switch (eventData.type) {
      case 'query_start':
        onQueryStart(eventData, {
          maxQueryCount: params.logSize || 100
        });
        break;
      case 'query_error':
      case 'query_result':
        onQueryEnd(eventData);
        break;
    }
  });
});
