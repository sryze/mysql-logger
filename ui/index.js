var queries = [];
var emptyMessage = document.getElementById('empty-message');
var tableContainer = document.getElementById('table-container');
var table = document.getElementById('table');
var infoPanel = document.getElementById('info-panel');

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
  if (!window.location.search) {
    return {};
  }
  var params = {};
  var queryString = window.location.search.substring(1);
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
  appendCell(row, query.index);
  appendCell(row, query.queryId);
  appendCell(row, query.database);
  appendCell(row, query.user);
  appendCell(row, query.query).className = 'query-cell';
  appendCell(row, query.startTimeString);
  return row;
}

function updateInfoPanelForQuery(query) {
  infoPanel.innerHTML = createQueryInfo(query);
}

function createHighlightedQuery(queryText) {
  return '<span class="code">' + queryText + '</span>';
}

function createQueryInfo(query) {
  var content = '<b>Query #' + query.index + ':</b><br>'
    + createHighlightedQuery(query.query)
    + '<br><br><b>Query ID:</b><br>'
    + query.queryId
    + '<br><br><b>Database:</b><br>'
    + query.database
    + '<br><br><b>User:</b><br>'
    + query.user
    + '<br><br><b>Started:</b><br>'
    + query.startTimeString;
  if (query.executionTime != null) {
    if (query.executionTime == 0) {
      content += '<br><br><b>Time:</b><br>&lt; 1 second';
    } else {
      content += '<br><br><b>Time:</b><br>' + query.executionTime + ' seconds';
    }
  }
  return content;
}

function onQueryStart(event) {
  if (queries.length == 0) {
    addClass(emptyMessage, 'hidden');
  }

  removeClass(table, 'hidden');

  var scrollShouldFollow = tableContainer.scrollTop
      >= tableContainer.scrollHeight - tableContainer.clientHeight - 30;
  var query = {
    index: queries.length + 1,
    database: event.database,
    user: event.user,
    query: event.query,
    queryId: event.query_id,
    startTime: event.time,
    startTimeString:
      event.time ? formatDateTime(new Date(event.time * 1000)) : ''
  };
  queries.push(query);

  var row = appendQueryRow(table, query);
  row.addEventListener('click', function() {
    if (hasClass(row, 'selected')) {
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
  });

  if (scrollShouldFollow) {
    tableContainer.scrollTop = tableContainer.scrollHeight;
  }
}

function onQueryEnd(event) {
  if (event.query_id == 0) {
    return;
  }

  var success = !event.error_code;
  for (var i = 0; i < queries.length; i++) {
    var query = queries[i];
    if (query.queryId == event.query_id) {
      addClass(table.rows[i + 1], success ? 'success' : 'error');
      query.executionTime = event.time - query.startTime;
      break;
    }
  }
}

window.addEventListener('DOMContentLoaded', function() {
  var params = getQueryStringParams();
  var host = params.host || window.location.hostname || 'localhost';
  var port = params.port || 13307;
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

    var loggerEvent = JSON.parse(event.data);
    switch (loggerEvent.type) {
      case 'query_start':
        onQueryStart(loggerEvent);
        break;
      case 'query_error':
      case 'query_result':
        onQueryEnd(loggerEvent);
        break;
    }
  });
});
