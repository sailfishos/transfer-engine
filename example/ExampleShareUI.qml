/******************************************************************************
Copyright (c) 2014 - 2021 Jolla Ltd.
Copyright (c) 2021 Open Mobile Platform LLC.

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer. Redistributions in binary
    form must reproduce the above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or other materials
    provided with the distribution. Neither the name of the Jolla Ltd. nor
    the names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

import QtQuick 2.6
import Sailfish.Silica 1.0
import org.nemomobile.thumbnailer 1.0
import Sailfish.TransferEngine 1.0

SilicaFlickable {
    id: root

    property var shareAction

    Component.onCompleted: {
        sailfishTransfer.loadConfiguration(shareAction.toConfiguration())
    }

    SailfishTransfer {
        id: sailfishTransfer
    }

    width: Screen.width
    height: Math.min(Screen.height, contentHeight)
    contentHeight: contentColumn.height

    Column {
        id: contentColumn

        width: parent.width
        spacing: Theme.paddingLarge
        bottomPadding: Theme.paddingLarge

        Thumbnail {
            width: Screen.width
            height: Screen.height / 2
            source: sailfishTransfer.source
            sourceSize.width: Screen.width
            sourceSize.height: Screen.height / 2
        }

        Label {
            width: Screen.width
            //: Label for example share UI
            //% "Example Test Share UI"
            text: qsTrId("example-test-share-ui-la-id")
            horizontalAlignment: Text.AlignHCenter
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            //% "Start"
            text: qsTrId("example-test-share-ui-la-start")

            onClicked: {
                sailfishTransfer.metadataStripped = true
                sailfishTransfer.userData = {
                    "description": "Random Text which can be what ever",
                    "accountId": sailfishTransfer.transferMethodInfo.accountId,
                    "scalePercent": 0.5
                }
                sailfishTransfer.start()
            }
        }
    }
}

