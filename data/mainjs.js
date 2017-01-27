//Объекты Vue
let panel_info = new Vue({
    el: '#info_network',
    data: {
        data_ip: '---',
        data_mask: '---',
        data_gateway: '---',
        data_hostname: '---'
    }
});
let panel_wifi = new Vue({
    el: '#info_wifi',
    data: {
        data_ssid: '---',
        data_rssi: '---',
    },
    computed: {
        obj_lv1: function () {
            console.log(this.data_rssi);
            if (this.data_rssi > -101)
                return {fill: '#25A9E0'};
            else
                return {fill: '#3A3A3C'}
        },
        obj_lv2: function () {
            console.log(this.data_rssi);
            if (this.data_rssi > -90)
                return {fill: '#25A9E0'};
            else
                return {fill: '#3A3A3C'}
        },
        obj_lv3: function () {
            console.log(this.data_rssi);
            if (this.data_rssi > -80)
                return {fill: '#25A9E0'};
            else
                return {fill: '#3A3A3C'}
        },
        obj_lv4: function () {
            console.log(this.data_rssi);
            if (this.data_rssi > -70)
                return {fill: '#25A9E0'};
            else
                return {fill: '#3A3A3C'}
        },
        obj_lv5: function () {
            console.log(this.data_rssi);
            if (this.data_rssi > -60)
                return {fill: '#25A9E0'};
            else
                return {fill: '#3A3A3C'}
        },
    }
});

//Запрос статичекой информации
fetch('staticstat.json')
    .then(checkStatus)
    .then(parseJSON)
    .then(function (data) {
        panel_info.data_ip = data['ip'];
        panel_info.data_mask = data['subnet'];
        panel_info.data_gateway = data['gateway'];
        panel_info.data_hostname = data['hostname'];
        panel_wifi.data_ssid = data['ssid'];
        reqwifiinfo();
    }).catch(function (error) {
    console.log('request failed', error)
});

//Запрос динамической информации
setInterval(function () {
    reqwifiinfo();
}, 5000);

function reqwifiinfo() {
    fetch('dynamicstat.json', {cache: 'no-cache'})
        .then(checkStatus)
        .then(parseJSON)
        .then(function (data) {
            panel_wifi.data_rssi = data['wifi_rssi'];
        }).catch(function (error) {
        console.log('request failed', error)
    });
}

function checkStatus(response) {
    if (response.status >= 200 && response.status < 300) {
        return response
    } else {
        let error = new Error(response.statusText);
        error.response = response;
        throw error
    }
}

function parseJSON(response) {
    return response.json()
}
