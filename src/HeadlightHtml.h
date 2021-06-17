const char webControlHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8" name="viewport" content="width=device-width,initial-scale=1" />
        <title>Trần Anh</title>
        <style>
            html {
                font-family: Arial;
                display: inline-block;
                text-align: center;
            }
            h1 {
                font-size: 1.9rem;
            }
            h2 {
                font-size: 1.5rem;
                text-align: left;
            }
            p {
                font-size: 1.2rem;
            }
            body {
                max-width: 400px;
                margin: 0 auto;
                padding-bottom: 25px;
            }
            .slider {
                border-radius: 25px;
                -webkit-appearance: none;
                margin: 14px;
                width: 360px;
                height: 25px;
                background: #4caf50;
                outline: 0;
                -webkit-transition: 0.2s;
                transition: opacity 0.2s;
            }
            .slider::-webkit-slider-thumb {
                border-radius: 25px;
                -webkit-appearance: none;
                appearance: none;
                width: 35px;
                height: 35px;
                background: #003249;
                cursor: pointer;
            }
            .slider::-moz-range-thumb {
                width: 35px;
                height: 35px;
                background: #003249;
                cursor: pointer;
            }
            .button {
                border-radius: 25px;
                background-color: #4caf50;
                border: none;
                color: #fff;
                padding: 15px 32px;
                text-align: center;
                text-decoration: none;
                display: inline-block;
                font-size: 16px;
                margin: 4px 2px;
                cursor: pointer;
            }
        </style>
        <body>
            <h1>Motor Controller</h1>
            <h2>1. Báo volt</h2>
            <p><span id="textVoltReport">Đang tải...</span></p>
            <br />
            <h2>2. Chỉnh đèn</h2>
            <p><span id="textSliderValue">Đang tải...</span></p>
            <p><input type="range" onchange="updateHeadlightPosition(this)" id="headlightPositionSlider" min="0" max="%MAX_POSITION%" value="%CURRENT_POSITION%" step="1" class="slider" /></p>
            <div id="tstatButtons">
                <br />
                <br />
                <input id="fastTopBtn" type="button" class="button" value="Đến mức cao" onclick="fastTopClick()" /><input type="button" class="button" value="Lưu mức cao" onclick="saveTopClick()" /><br />
                <br />
                <input id="fastBotBtn" type="button" class="button" value="Đến mức thấp" onclick="fastBottomClick()" /> <input type="button" class="button" value="Lưu mức thấp" onclick="saveBottomClick()" />
            </div>
            <script>
                setInterval(function() {
                    currentPosition();
                    voltReport();
                }, 500);

                function updateHeadlightPosition(t) {
                    var e = document.getElementById("headlightPositionSlider").value;
                    var percent = Math.round(parseInt(e) * 100 / %MAX_POSITION% );
                    document.getElementById("textSliderValue").innerHTML = percent;
                    var n = new XMLHttpRequest();
                    n.open("GET", "/adjust-headlight?value=" + e, !0), n.send();
                }

                function saveTopClick() {
                    var t = document.getElementById("headlightPositionSlider").value;
                    var percent = Math.round(parseInt(t) * 100 / %MAX_POSITION% );
                    document.getElementById("textSliderValue").innerHTML = percent;
                    var e = new XMLHttpRequest();
                    e.open("GET", "/save-top?value=" + t, !0), e.send();
                }

                function saveBottomClick() {
                    var t = document.getElementById("headlightPositionSlider").value;
                    var percent = Math.round(parseInt(t) * 100 / %MAX_POSITION% );
                    document.getElementById("textSliderValue").innerHTML = percent;
                    var e = new XMLHttpRequest();
                    e.open("GET", "/save-bottom?value=" + t, !0), e.send();
                }

                function fastTopClick() {
                    var t = new XMLHttpRequest();
                    t.open("GET", "/fast-top", !0), t.send();
                }

                function fastBottomClick() {
                    var t = new XMLHttpRequest();
                    t.open("GET", "/fast-bottom", !0), t.send();
                }

                function currentPosition() {
                    var xhttp = new XMLHttpRequest();
                    xhttp.onreadystatechange = function() {
                        if (this.readyState == 4 && this.status == 200) {
                            //Calculate by percent to avoid noticable adc noise
                            var percent = Math.round(parseInt(this.responseText) * 100 / %MAX_POSITION% );
                            document.getElementById("textSliderValue").innerHTML = "Mức hiện tại: " + percent;
                            document.getElementById("headlightPositionSlider").value = Math.round(percent / 100 * %MAX_POSITION% );
                            document.getElementById("fastTopBtn").value = "Đến mức cao (" + Math.round(%TOP_POSITION% * 100 / %MAX_POSITION% ) + ")";
                            document.getElementById("fastBotBtn").value = "Đến mức thấp (" +  Math.round(%BOT_POSITION% * 100 / %MAX_POSITION% ) + ")";

                        }
                    };
                    xhttp.open("GET", "currentPosition", true);
                    xhttp.send();
                }

                function voltReport() {
                    var xhttp = new XMLHttpRequest();
                    xhttp.onreadystatechange = function() {
                      if (this.readyState == 4 && this.status == 200) {
                            document.getElementById("textVoltReport").innerHTML =  this.responseText;
                        }
                    };
                    xhttp.open("GET", "volt-report", true);
                    xhttp.send();
                }
            </script>
        </body>
    </head>
</html>

)rawliteral";
