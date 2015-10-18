#
#   This file is part of JasonQt
#
#   Copyright: Jason
#
#   Contact email: 188080501@qq.com
#
#   GNU Lesser General Public License Usage
#   Alternatively, this file may be used under the terms of the GNU Lesser
#   General Public License version 2.1 or version 3 as published by the Free
#   Software Foundation and appearing in the file LICENSE.LGPLv21 and
#   LICENSE.LGPLv3 included in the packaging of this file. Please review the
#   following information to ensure the GNU Lesser General Public License
#   requirements will be met: https://www.gnu.org/licenses/lgpl.html and
#   http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
#

CONFIG += c++11
CONFIG += c++14

INCLUDEPATH += $$PWD/ \
    $$PWD/JasonQt_Sjf

# Jsf device
exists($$PWD/JasonQt_Sjf/JasonQt_SjfServer.h) {
    SOURCES += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfDevice.cpp

    HEADERS += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfDevice.h
}

# Jsf server
exists($$PWD/JasonQt_Sjf/JasonQt_SjfServer.h) {
    SOURCES += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfServer.cpp

    HEADERS += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfServer.h
}

# Sjf client
exists($$PWD/JasonQt_Sjf/JasonQt_SjfClient.h) {
    SOURCES += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfClient.cpp

    HEADERS += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfClient.h
}

# Sjf for tcp device
exists($$PWD/JasonQt_Sjf/JasonQt_SjfForTcpServer.h) {
    HEADERS += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfForTcpDevice.h
}

# Sjf for tcp server
exists($$PWD/JasonQt_Sjf/JasonQt_SjfForTcpServer.h) {
    SOURCES += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfForTcpServer.cpp

    HEADERS += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfForTcpServer.h
}

# sjf for tcp client
exists($$PWD/JasonQt_Sjf/JasonQt_SjfForTcpClient.h) {
    SOURCES += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfForTcpClient.cpp

    HEADERS += \
        $$PWD/JasonQt_Sjf/JasonQt_SjfForTcpClient.h
}
