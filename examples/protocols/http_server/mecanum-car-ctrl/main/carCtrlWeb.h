#ifndef _CAR_CTRL_WEB_H_
#define _CAR_CTRL_WEB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/param.h>
#include "freertos/FreeRTOS.h"

char* getCarCtrlWebPage() {
    char* strWebPage = malloc(1024 * 8);
    char* strHtmlTemplate = "\
<html> \
<head> \
    <style> \
        html { \
            font-family: Arial; \
            display: inline-block; \
            margin: 0px auto; \
            text-align: center; \
        } \
        .btnSpeedUp { \
            background-color: orange; \
            color: black; \
            padding: 30px 67px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            cursor: pointer; \
        } \
        .btnSlowDown { \
            background-color: green; \
            color: black; \
            padding: 30px 53px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            cursor: pointer; \
        } \
        .btnLeftFront { \
            background-color: #5882FA; \
            color: black; \
            padding: 30px 72px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            margin-left: 0px; \
            cursor: pointer; \
        } \
        .btnForward { \
            background-color: cyan; \
            color: black; \
            padding: 30px 76px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            cursor: pointer; \
        } \
        .btnRightFront { \
            background-color: #5882FA; \
            color: black; \
            padding: 30px 56px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            cursor: pointer; \
        } \
        .btnCarLeft { \
            background-color: cyan; \
            color: black; \
            padding: 30px 93px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            cursor: pointer; \
        } \
        .btnStop { \
            background-color: red; \
            color: black; \
            padding: 30px 108px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            cursor: pointer; \
        } \
        .btnCarRight { \
            background-color: cyan; \
            color: black; \
            padding: 30px 76px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            cursor: pointer; \
        } \
        .btnTurnLeft { \
            background-color: #5882FA; \
            color: black; \
            padding: 30px 78px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            cursor: pointer; \
        } \
        .btnBack { \
            background-color: cyan; \
            color: black; \
            padding: 30px 64px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            cursor: pointer; \
        } \
        .btnTurnRight { \
            background-color: #5882FA; \
            color: black; \
            padding: 30px 60px; \
            text-align: center; \
            display: inline-block; \
            font-size: 40px; \
            margin: 4px 2px; \
            cursor: pointer; \
        } \
\
    </style> \
</head> \
<body> \
    <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/> \
    <h1>Mecanum Car Controller :></h1> \
    <br/> \
    <p> \
        <button class=\"btnSpeedUp\" onclick=\"speedUp()\">SpeedUp</button> \
        <button class=\"btnSlowDown\" onclick=\"slowDown()\">SlowDown</button> \
    </p> \
    <p> \
        <button class=\"btnLeftFront\" onclick=\"leftFront()\">LeftFront</button> \
        <button class=\"btnForward\" onclick=\"carForward()\">Forward</button> \
        <button class=\"btnRightFront\" onclick=\"rightFront()\">RightFront</button> \
    </p> \
    <br/> \
    <p> \
        <button class=\"btnCarLeft\" onclick=\"carLeft()\">GoLeft</button> \
        <button class=\"btnStop\" onclick=\"carStop()\">Stop</button> \
        <button class=\"btnCarRight\" onclick=\"carRight()\">GoRight</button> \
    </p> \
    <br/> \
    <p> \
        <button class=\"btnTurnLeft\" onclick=\"turnLeft()\">TurnLeft</button> \
        <button class=\"btnBack\" onclick=\"carBackward()\">Backward</button> \
        <button class=\"btnTurnRight\" onclick=\"turnRight()\">TurnRight</button> \
    </p> \
    <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/> \
    <a href=\"?wifi_config_page\" style=\"font-size:23px\">WiFi Config </a> \
    <br/> \
    <script type=\"text/javascript\"> \
        function speedUp() { \
            fetch(\"http://%s/?speed_up\"); \
        } \
        function slowDown() { \
            fetch(\"http://%s/?slow_down\"); \
        } \
        function leftFront() { \
            fetch(\"http://%s/?left_front\"); \
        } \
        function carForward() { \
            fetch(\"http://%s/?car_forward\"); \
        } \
        function rightFront() { \
            fetch(\"http://%s/?right_front\"); \
        } \
        function carLeft() { \
            fetch(\"http://%s/?car_left\"); \
        } \
        function carStop() { \
            fetch(\"http://%s/?car_stop\"); \
        } \
        function carRight() { \
            fetch(\"http://%s/?car_right\"); \
        } \
        function turnLeft() { \
            fetch(\"http://%s/?turn_left\"); \
        } \
        function carBackward() { \
            fetch(\"http://%s/?car_backward\"); \
        } \
        function turnRight() { \
            fetch(\"http://%s/?turn_right\"); \
        } \
    </script> \
</body> \
</html> \
";
    char* domainName = getDomainName();
    sprintf(strWebPage, strHtmlTemplate, domainName,
        domainName, domainName, domainName, domainName, domainName,
        domainName, domainName, domainName, domainName, domainName);
    return strWebPage;
}

#ifdef __cplusplus
}
#endif

#endif /* _CAR_CTRL_WEB_H_ */
