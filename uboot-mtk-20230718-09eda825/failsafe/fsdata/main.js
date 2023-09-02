
function ajax(opt) {
    var xmlhttp;

    if (window.XMLHttpRequest) {
        //  IE7+, Firefox, Chrome, Opera, Safari
        xmlhttp = new XMLHttpRequest();
    } else {
        // IE6, IE5
        xmlhttp = new ActiveXObject('Microsoft.XMLHTTP');
    }

    xmlhttp.upload.addEventListener('progress', function (e) {
        if (opt.progress)
            opt.progress(e)
    })

    xmlhttp.onreadystatechange = function() {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
            if (opt.done)
                opt.done(xmlhttp.responseText);
        }
    }

    if (opt.timeout)
        xmlhttp.timeout = opt.timeout;

    var method = 'GET';

    if (opt.data)
        method = 'POST'

    xmlhttp.open(method, opt.url);
    xmlhttp.send(opt.data);
}

function getversion() {
    ajax({
        url: '/version',
        done: function(version) {
            document.getElementById('version').innerHTML = version
        }
    })
}

function upload(name) {
    var file = document.getElementById('file').files[0]
    if (!file)
        return

    document.getElementById('form').style.display = 'none';
    document.getElementById('hint').style.display = 'none';

    var form = new FormData();
    form.append(name, file);

    ajax({
        url: '/upload',
        data: form,
        done: function(resp) {
            if (resp == 'fail') {
                location = '/fail.html';
            } else {
                const info = resp.split(' ');

                document.getElementById('size').style.display = 'block';
                document.getElementById('size').innerHTML = 'Size: ' + info[0];

                document.getElementById('md5').style.display = 'block';
                document.getElementById('md5').innerHTML = 'MD5: ' + info[1];

                document.getElementById('upgrade').style.display = 'block';
            }
        },
        progress: function(e) {
            var percentage = parseInt(e.loaded / e.total * 100)
            document.getElementById('bar').setAttribute('style', '--percent: ' + percentage);
        }
    })
}
