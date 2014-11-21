/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

.pragma library

var qmlProfilerModelProxy = 0;
var zoomControl = 0;

//draw background of the graph
function drawGraph(canvas, ctxt)
{
    ctxt.fillStyle = "#eaeaea";
    ctxt.fillRect(0, 0, canvas.width, canvas.height);
}

//draw the actual data to be graphed
function drawData(canvas, ctxt)
{
    if (!zoomControl || !qmlProfilerModelProxy || qmlProfilerModelProxy.isEmpty())
        return;

    for (var modelIndex = 0; modelIndex < qmlProfilerModelProxy.modelCount(); ++modelIndex) {
        for (var ii = canvas.offset; ii < qmlProfilerModelProxy.count(modelIndex);
             ii += canvas.increment) {

            var xx = (qmlProfilerModelProxy.startTime(modelIndex,ii) - zoomControl.traceStart) *
                     canvas.spacing;

            var eventWidth = qmlProfilerModelProxy.duration(modelIndex,ii) * canvas.spacing;

            if (eventWidth < 1)
                eventWidth = 1;

            xx = Math.round(xx);

            var itemHeight = qmlProfilerModelProxy.relativeHeight(modelIndex, ii) *
                    canvas.blockHeight;
            var yy = (modelIndex + 1) * canvas.blockHeight - itemHeight ;

            ctxt.fillStyle = qmlProfilerModelProxy.color(modelIndex, ii);
            ctxt.fillRect(xx, canvas.bump + yy, eventWidth, itemHeight);
        }
    }
}

function drawBindingLoops(canvas, ctxt) {
    ctxt.strokeStyle = "orange";
    ctxt.lineWidth = 2;
    var radius = 1;
    for (var modelIndex = 0; modelIndex < qmlProfilerModelProxy.modelCount(); ++modelIndex) {
        for (var ii = canvas.offset - canvas.increment; ii < qmlProfilerModelProxy.count(modelIndex);
             ii += canvas.increment) {
            if (qmlProfilerModelProxy.bindingLoopDest(modelIndex,ii) >= 0) {
                var xcenter = Math.round(qmlProfilerModelProxy.startTime(modelIndex,ii) +
                                         qmlProfilerModelProxy.duration(modelIndex,ii) -
                                         zoomControl.traceStart) * canvas.spacing;
                var ycenter = Math.round(canvas.bump + canvas.blockHeight * modelIndex +
                                         canvas.blockHeight / 2);
                ctxt.beginPath();
                ctxt.arc(xcenter, ycenter, radius, 0, 2*Math.PI, true);
                ctxt.stroke();
            }
        }
    }
}

function drawNotes(canvas, ctxt)
{
    if (!zoomControl || !qmlProfilerModelProxy || qmlProfilerModelProxy.noteCount() === 0)
        return;

    var spacing = canvas.width / zoomControl.traceDuration;
    // divide canvas height in 7 parts: margin, 3*line, space, dot, margin
    var vertSpace = (canvas.height - canvas.bump) / 7;

    ctxt.strokeStyle = "orange";
    ctxt.lineWidth = 2;
    for (var i = 0; i < qmlProfilerModelProxy.noteCount(); ++i) {
        var timelineModel = qmlProfilerModelProxy.noteTimelineModel(i);
        var timelineIndex = qmlProfilerModelProxy.noteTimelineIndex(i);
        if (timelineIndex === -1)
            continue;
        var start = Math.max(qmlProfilerModelProxy.startTime(timelineModel, timelineIndex),
                             zoomControl.traceStart);
        var end = Math.min(qmlProfilerModelProxy.endTime(timelineModel, timelineIndex),
                           zoomControl.traceStart);
        var annoX = Math.round(((start + end) / 2 - zoomControl.traceStart) * spacing);

        ctxt.moveTo(annoX, canvas.bump + vertSpace)
        ctxt.lineTo(annoX, canvas.bump + vertSpace * 4)
        ctxt.stroke();
        ctxt.moveTo(annoX, canvas.bump + vertSpace * 5);
        ctxt.lineTo(annoX, canvas.bump + vertSpace * 6);
        ctxt.stroke();
    }
}

function drawTimeBar(canvas, ctxt)
{
    if (!zoomControl)
        return;

    var width = canvas.width;
    var height = 10;

    var initialBlockLength = 120;
    var timePerBlock = Math.pow(2, Math.floor( Math.log( zoomControl.traceDuration / width *
                       initialBlockLength ) / Math.LN2 ) );
    var pixelsPerBlock = timePerBlock * canvas.spacing;
    var pixelsPerSection = pixelsPerBlock / 5;
    var blockCount = width / pixelsPerBlock;

    var realStartTime = Math.floor(zoomControl.traceStart / timePerBlock) * timePerBlock;
    var realStartPos = (zoomControl.traceStart - realStartTime) * canvas.spacing;

    var timePerPixel = timePerBlock/pixelsPerBlock;

    ctxt.fillStyle = "#000000";
    ctxt.font = "6px sans-serif";

    ctxt.fillStyle = "#cccccc";
    ctxt.fillRect(0, 0, width, height);
    for (var ii = 0; ii < blockCount+1; ii++) {
        var x = Math.floor(ii*pixelsPerBlock - realStartPos);

        // block boundary
        ctxt.strokeStyle = "#525252";
        ctxt.beginPath();
        ctxt.moveTo(x, height/2);
        ctxt.lineTo(x, height);
        ctxt.stroke();

        // block time label
        ctxt.fillStyle = "#000000";
        var timeString = prettyPrintTime((ii+0.5)*timePerBlock + realStartTime);
        ctxt.textAlign = "center";
        ctxt.fillText(timeString, x + pixelsPerBlock/2, height/2 + 3);
    }

    ctxt.fillStyle = "#808080";
    ctxt.fillRect(0, height-1, width, 1);
}

function prettyPrintTime( t )
{
    if (t <= 0) return "0";
    if (t<1000) return t+" ns";
    t = t/1000;
    if (t<1000) return t+" μs";
    t = Math.floor(t/100)/10;
    if (t<1000) return t+" ms";
    t = Math.floor(t)/1000;
    if (t<60) return t+" s";
    var m = Math.floor(t/60);
    t = Math.floor(t - m*60);
    return m+"m"+t+"s";
}
