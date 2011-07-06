/**************************************************************************

**

**  This  file  is  part  of  Qt  Creator

**

**  Copyright  (c)  2011  Nokia  Corporation  and/or  its  subsidiary(-ies).

**

**  Contact:  Nokia  Corporation  (info@qt.nokia.com)

**

**  No  Commercial  Usage

**

**  This  file  contains  pre-release  code  and  may  not  be  distributed.

**  You  may  use  this  file  in  accordance  with  the  terms  and  conditions

**  contained  in  the  Technology  Preview  License  Agreement  accompanying

**  this  package.

**

**  GNU  Lesser  General  Public  License  Usage

**

**  Alternatively,  this  file  may  be  used  under  the  terms  of  the  GNU  Lesser

**  General  Public  License  version  2.1  as  published  by  the  Free  Software

**  Foundation  and  appearing  in  the  file  LICENSE.LGPL  included  in  the

**  packaging  of  this  file.   Please  review  the  following  information  to

**  ensure  the  GNU  Lesser  General  Public  License  version  2.1  requirements

**  will  be  met:  http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.

**

**  In  addition,  as  a  special  exception,  Nokia  gives  you  certain  additional

**  rights.   These  rights  are  described  in  the  Nokia  Qt  LGPL  Exception

**  version  1.1,  included  in  the  file  LGPL_EXCEPTION.txt  in  this  package.

**

**  If  you  have  questions  regarding  the  use  of  this  file,  please  contact

**  Nokia  at  info@qt.nokia.com.

**

**************************************************************************/

#ifndef QMLDESIGNER_QT5INFORMATIONNODEINSTANCESERVER_H
#define QMLDESIGNER_QT5INFORMATIONNODEINSTANCESERVER_H

#include "qt5nodeinstanceserver.h"

namespace QmlDesigner {

class Qt5InformationNodeInstanceServer : public Qt5NodeInstanceServer
{
    Q_OBJECT
public:
    explicit Qt5InformationNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);

    void reparentInstances(const ReparentInstancesCommand &command);
    void clearScene(const ClearSceneCommand &command);
    void createScene(const CreateSceneCommand &command);
    void completeComponent(const CompleteComponentCommand &command);

protected:
    void collectItemChangesAndSendChangeCommands();
    void sendChildrenChangedCommand(const QList<ServerNodeInstance> childList);

private:
    QSet<ServerNodeInstance> m_parentChangedSet;
    QList<ServerNodeInstance> m_completedComponentList;
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_QT5INFORMATIONNODEINSTANCESERVER_H
