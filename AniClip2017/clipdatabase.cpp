#include "clipdatabase.h"
#include "logger.h"

#include <QDebug>
#include <QString>
#include <QTextStream>

#include <QFile>
#include <QDir>
#include <QtAlgorithms>


bool compareTags(const QString &s1, const QString &s2) {

    // ignore common prefix..
    int i = 0;
    while ((i < s1.length()) && (i < s2.length()) && (s1.at(i).toLower() == s2.at(i).toLower())) {
        ++i;
    }

    // Look at Next item
    //++i;

    // something left to compare?
    if ((i < s1.length()) && (i < s2.length()))
    {
       // get relevant/signficant number string for s1
       int k = i;
       QString n1 = "";
       while ((k < s1.length()) && (s1.at(k).isNumber())) {
           n1 += s1.at(k);
           ++k;
       }

       // get relevant/signficant number string for s2
       k = i;
       QString n2 = "";
       while ((k < s2.length()) && (s2.at(k).isNumber())) {
           n2 += s2.at(k);
           ++k;
       }

       // got two numbers to compare?
       if (!n1.isEmpty() && !n2.isEmpty())
       {
           return n1.toInt() < n2.toInt();
       }
       else {
           // not a number has to win over a number.. number could have ended earlier... same prefix..
          if (!n1.isEmpty()) {
              return false;
          }
          else if (!n2.isEmpty()) {
              return true;
          }
          else {
            return s1.at(i) < s2.at(i);
          }
       }
    }
    else {
       // shortest string wins
       return s1.length() < s2.length();
    }

    return false;
}

bool compareGroups(TagGroup *t1, TagGroup *t2) {
    QString s1 = t1->getName();
    QString s2 = t2->getName();
    return compareTags(s1, s2);
}

TagGroup::TagGroup(logger::Logger *nLog, QObject *parent) : QObject(parent),
log(nLog)
{

}

void TagGroup::setName(QString nName) {
    groupName = nName;
}

bool TagGroup::addTag(QString nTag) {
    bool addSuccess_flag = false;
    if (!nTag.isEmpty()) {
        if (!groupTags.contains(nTag)) {
            groupTags.append(nTag);
            addSuccess_flag = true;
        }
    }
    return addSuccess_flag;
}

bool TagGroup::addTags(QStringList nTags) {
    bool addSuccess_flag = true;

    groupTags.append(nTags);
    groupTags.removeDuplicates();
    groupTags.removeAll("");

    return addSuccess_flag;
}

bool TagGroup::removeTag(QString nTag) {
    bool removeSuccess_flag = false;
    if (groupTags.removeOne(nTag)) {
        removeSuccess_flag = true;
    }
    return removeSuccess_flag;
}

bool TagGroup::addGroup(TagGroup oGroup) {
    QStringList nTags = oGroup.getTags();
    int numAdded = 0;
    for (int i = 0; i < nTags.count(); i++) {
        if (addTag(nTags.at(i))) {
            numAdded++;
        }
    }
    log->info(QString("Added %1 new tags to %2 group").arg(numAdded).arg(groupName));

    return true;
}

bool TagGroup::sortThis() {
    qSort(groupTags.begin(), groupTags.end(), compareTags);
    return true;
}

QStringList TagGroup::getTags() {
    return groupTags;
}

QString TagGroup::getName() {
    return groupName;
}

TagManager::TagManager(logger::Logger *nLog, QObject *parent) : QObject(parent),
    log(nLog)
{

}

bool TagManager::readTagLine(QString line) {
    bool rFlag = true;
    QStringList split = line.split(":");

    if (split.count() == 2) {
        QString nName = "";
        QString nTags = "";
        QStringList tagList;

        if (split.at(0).startsWith("name=")) {
            nName = split.at(0).right(split.at(0).length() - 5);
        }

        if (split.at(1).startsWith("tags=")) {
            nTags = split.at(1).right(split.at(1).length() - 5);
            tagList = nTags.split("|");
        }

        if (!nTags.isEmpty() && !nName.isEmpty()) {
            TagGroup *nGroup = new TagGroup(log, this);
            nGroup->setName(nName);
            groups.append(nGroup);
            addTags(tagList, nGroup->getName());
            log->info(QString("Created new TagGroup %1. Added %2 tags.").arg(nGroup->getName()).arg(nGroup->getTags().count()));
        }
    }
    else {
        log->warn(QString("Invalid tag line: \"%1\"").arg(line));
        rFlag = false;
    }

    return rFlag;
}

bool TagManager::addTag(QString tag, QString groupName) {
    if (groupName.isEmpty()) {
        groupName = "General";
    }
    else {
        getGroup("General")->addTag(tag);
    }

    TagGroup *nGroup = getGroup(groupName);
    return nGroup->addTag(tag);
}

bool TagManager::addTags(QStringList tags, QString groupName) {

    if (groupName.isEmpty()) {
        groupName = "General";
    }
    else {
        getGroup("General")->addTags(tags);
    }

    TagGroup *nGroup = getGroup(groupName);
    return nGroup->addTags(tags);
}

TagGroup* TagManager::addGroup(QString nGroupName) {
    TagGroup *rGroup = NULL;
    if (!containsGroup(nGroupName)) {
        TagGroup *nGroup = new TagGroup(log, this);
        nGroup->setName(nGroupName);
        groups.append(nGroup);
        rGroup = nGroup;
    }

    return rGroup;
}

TagGroup* TagManager::getGroup(QString groupName) {
    TagGroup *rGroup = NULL;
    bool rFlag = false;

    for (int i = 0; i < groups.count() && !rFlag; i++) {
        if (groupName == groups.at(i)->getName()) {
            rGroup = groups.at(i);
            rFlag = true;
        }
    }

    if (rGroup == NULL) {
        rGroup = addGroup(groupName);
    }

    return rGroup;
}

bool TagManager::sortThis() {
    qSort(groups.begin(), groups.end(), compareGroups);

    for (int i = 0; i < groups.count(); i++) {
        groups.at(i)->sortThis();
    }

    return true;
}

bool TagManager::containsGroup(QString nName) {
    bool rFlag = false;

    for (int i = 0; i < groups.count() && !rFlag; i++) {
        if (nName == groups.at(i)->getName()) {
            rFlag = true;
        }
    }

    return rFlag;
}

Clip::Clip(logger::Logger *nLog, QObject *parent) : QObject(parent),
    log(nLog)
{

}

void Clip::setShowName(QString nShowName) {
    showName = nShowName;
}

void Clip::setEpNum(int nEpNum) {
    epNum = nEpNum;
}

void Clip::setTimeBound(TimeBound nTimeBound) {
    bounds.startTime    = nTimeBound.startTime;
    bounds.endTime      = nTimeBound.endTime;
}

void Clip::writeClipToFile(QTextStream &nStream) {

    nStream << showName << "[|]" << epNum << "[|]" << bounds.startTime.toString("hh:mm:ss") << "-" << bounds.endTime.toString("hh:mm:ss") << "[|]";
    nStream << season << "[|]" << year << "[|]" << tags.join("|") << "[|]" << localSrc << "[|]" << link << "[|]" << note << endl;
}

bool Clip::compareClip(Clip *oClip) {
    bool rFlag = false;
    if (    showName == oClip->showName &&
            epNum    == oClip->epNum    &&
            bounds.startTime == oClip->bounds.startTime &&
            bounds.endTime   == oClip->bounds.endTime) {
        rFlag = true;
    }

    return rFlag;
}

ShowList::ShowList(logger::Logger *nLog, QObject *parent) : QObject(parent),
    log(nLog),
    showName()
{

}

void ShowList::writeListToFile(QTextStream &nStream) {

    nStream << "\t#" << showName << endl;
    for (int i = 0; i < clips.count(); i++) {
        nStream << "\t";
        clips.at(i)->writeClipToFile(nStream);
    }

}

bool ShowList::addClip(Clip *nClip) {
    bool found_flag = false;
    bool rFlag = false;
    for (int i = 0; i < clips.count() && !found_flag; i++) {
        if (clips.at(i)->compareClip(nClip)) {
            found_flag = true;
        }
    }

    if (!found_flag) {
        insertClip(nClip);
        rFlag = true;
    }

    return rFlag;
}

QString ShowList::getName() {
    return showName;
}

void ShowList::setName(QString nName) {
    showName = nName;
}

int ShowList::getClipCount() {
    return clips.count();
}

void ShowList::insertClip(Clip *nClip) {
    bool clipInserted_flag = false;
    for (int i = 0; i < clips.count() && !clipInserted_flag; i++) {
        if (clips.at(i)->epNum > nClip->epNum) {
            clips.insert(i, nClip);
            clipInserted_flag = true;
        }
        else if (clips.at(i)->epNum == nClip->epNum) {
            int startTimeDif = clips.at(i)->bounds.startTime.secsTo(nClip->bounds.startTime);
            int endTimeDif = clips.at(i)->bounds.endTime.secsTo(nClip->bounds.endTime);
            if (startTimeDif < 0) {
                clips.insert(i, nClip);
                clipInserted_flag = true;
            }
            else if (startTimeDif == 0) {
                if (endTimeDif < 0) {
                    clips.insert(i, nClip);
                    clipInserted_flag = true;
                }
            }
        }
    }

    if (!clipInserted_flag) {
        clips.append(nClip);
    }
}

ClipList::ClipList(logger::Logger *nLog, QObject *parent) : QObject(parent),
    log(nLog),
    isVisible_flag(true)
{

}

bool ClipList::addClip(Clip* nClip) {
\
    ShowList *cShow = NULL;

    for (int i = 0; i < shows.count(); i++) {
        if (shows.at(i)->getName() == nClip->showName) {
            cShow = shows.at(i);
        }
    }

    if (cShow == NULL) {
        cShow = new ShowList(log, this);
        cShow->setName(nClip->showName);
        shows.append(cShow);
    }

    return cShow->addClip(nClip);
}

void ClipList::writeListToFile(QTextStream &nStream) {

    nStream << "List::" << listName << endl << "{" << endl << endl;

    for (int i = 0; i < shows.count(); i++) {
        shows.at(i)->writeListToFile(nStream);
        nStream << endl;
    }

    nStream << "}" << endl << endl;

}

void ClipList::setVisible(bool isVisible) {
   isVisible_flag = isVisible;
}

bool ClipList::isVisible() {
    return isVisible_flag;
}


QString ClipList::getName() {
    return listName;
}

void ClipList::setName(QString nName) {
    listName = nName;
}

int ClipList::getClipCount() {
    int totalClips = 0;
    for (int i = 0; i < shows.count(); i++) {
        totalClips += shows.at(i)->getClipCount();
    }

    return totalClips;
}

ShowList* ClipList::getShowList(QString show_name) {
    ShowList *rShow = NULL;
    bool rFlag = false;

    for (int i = 0; i < shows.count() && !rFlag; i++) {
        ShowList *cShow = shows.at(i);

        if (cShow->getName() == show_name) {
            rFlag = true;
            rShow = cShow;
        }
    }

    return rShow;
}

ClipDatabase::ClipDatabase(logger::Logger *nLog, QObject *parent) : QObject(parent),
    log(nLog),
    main_list(NULL),
    existingShows(),
    used_clips(),
    sub_lists(),
    tagManager(NULL),
    clips_filename(),
    tags_filename(),
    shows_filename()
{
    tagManager = new TagManager(nLog, this);
    tags_filename = "activeTagList.txt";
    shows_filename = "activeShowList.txt";
    clips_filename = "activeClipDB.txt";

    initMainList();
}

bool ClipDatabase::init(QString config_filename) {
    bool initSuccess_flag = true;

    if (log != NULL) {

        if (readConfig(config_filename)) {
            log->info("ClipDatabase.init: Read config file.");
        }
        else {
            log->err("ClipDatabase.init: Failed to read config file");
            initSuccess_flag = false;
        }

        if (loadTagList(tags_filename)) {
            log->info(QString("ClipDatabase.init: Loaded Tag file \"%1\"").arg(tags_filename));
        }
        else {
            log->warn(QString("ClipDatabase.init: Failed to load Tag File \"%1\".").arg(tags_filename));
        }

        if (loadShowList(shows_filename)) {
            log->info(QString("Clipdatabase.init: Loaded Show File \"%1\"").arg(shows_filename));
        }
        else {
            log->warn(QString("ClipDatabase.init: Failed to load Show File \"%1\".").arg(shows_filename));
        }

        if (loadClips(clips_filename)) {
            log->info(QString("ClipDatabase.init: Loaded Clip File \"%1\"").arg(clips_filename));
        }
        else {
            log->warn(QString("ClipDatabase.init: Failed to load Clip File \"%1\"").arg(clips_filename));
        }
    }
    else {
        qDebug () << "ERROR - ClipDatabase.init :: Logger not valid.";
        initSuccess_flag = false;
    }

    return initSuccess_flag;
}

bool ClipDatabase::readConfig(QString config_filename) {
    bool readSuccess_flag = true;

    QFile dbConfig(config_filename);
    if (dbConfig.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream tStream(&dbConfig);

        while(!tStream.atEnd()) {
            QString line = tStream.readLine();

            if (!line.isEmpty() && !line.startsWith('#')) {
                QStringList lineSplit = line.split(QRegExp("\\s+"));

                if (lineSplit.count() >= 2) {
                    QString id = lineSplit.at(0).trimmed();
                    QString input = lineSplit.at(1).trimmed();

                    if (id == "clips_filename") {
                        clips_filename = input;
                    }
                    else if(id == "tags_filename") {
                        tags_filename = input;
                    }
                    else if (id == "shows_filename") {
                        shows_filename = input;
                    }

                }
            }
        }

    }
    else {
        log->err(QString("QFile::%1").arg(dbConfig.errorString()));
        log->err(QString("Clipdatabase.init: Failed to open config file \"%1\".").arg(config_filename));
        readSuccess_flag = false;
    }

    return readSuccess_flag;

}

void ClipDatabase::selfTest() {
    // Add new Tag - no group

    // Add new Tag - 1 group

    // Add new Tag - multiple groups

    // Add new Clip - no lists

    // Add new Clip - multiple lists

    // Add new show
}

void ClipDatabase::save() {

    saveClips();
    saveShows();
    saveTags();

    writeBackup();
}

void ClipDatabase::saveClips() {
    log->info(QString("Saving ClipDatabase to file %1.").arg(clips_filename));
    QFile clipFile(clips_filename);
    if (clipFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&clipFile);

        out << "#ClipList | " << QDateTime::currentDateTime().toString("dd MMM YYYY mm:ss") << endl;

        main_list->writeListToFile(out);

        for (int i =0 ;i < sub_lists.count(); i++) {
            sub_lists.at(i)->writeListToFile(out);
        }

        clipFile.close();
    }
}

void ClipDatabase::saveShows() {
    log->info(QString("Saving ShowList to file %1.").arg(shows_filename));
    QFile showFile(shows_filename);
    if (showFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&showFile);

        out << "#ShowList | " << QDateTime::currentDateTime().toString("dd MMM YYYY mm:ss") << endl;
        for (int i = 0; i < existingShows.count(); i++) {
            out << existingShows.at(i) << endl;
        }
        showFile.close();
    }
}

void ClipDatabase::saveTags() {
    log->info(QString("Saving TagList to file %1.").arg(tags_filename));
    QFile tagsFile(tags_filename);
    if (tagsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&tagsFile);

        out << "#TagList | " << QDateTime::currentDateTime().toString("dd MMM YYYY mm:ss") << endl << endl;

        for (int i = 0; i < tagManager->groups.count(); i++) {
            TagGroup* cGroup = tagManager->groups.at(i);
            out << "name=" <<cGroup->getName() << ":tags=";
            QStringList tags = cGroup->getTags();
            out << tags.at(0);
            for (int j = 1; j < tags.count(); j++) {
                out <<"|" << tags.at(j);
            }
            out << endl;

        }

        tagsFile.close();
    }
}

void ClipDatabase::writeBackup() {
    QDir cDir;
    QString tBackup = QString("/backup");
    QString nBackup = QString("/backup_%1").arg(QDateTime::currentDateTime().toString("ddMMyy_mmss"));
    cDir.setPath(QDir::currentPath() + tBackup);

    if (!cDir.exists()) {
        if (!cDir.mkdir(".")) {
            log->err(QString("Unable to create backup folder \"%1\"").arg(tBackup));
        }
    }

    cDir.setPath(QDir::currentPath() + tBackup + nBackup);
    if (!cDir.mkdir(".")) {
        log->err(QString("Unable to create backup folder \"%1\"").arg(nBackup));
    }

    QFile clipFile(clips_filename);
    if (clipFile.exists()) {
        QString bFile = QString(QDir::currentPath() + tBackup + nBackup + "/" + clips_filename);
        log->info(QString("Backup %1").arg(bFile));
        if (!clipFile.copy(bFile)) {
            log->err(QString("Unable to backup ClipDB %1").arg(bFile));
        }
    }
    else {
        log->err(QString("Could not find file \"%1\" for backup.").arg(clips_filename));
    }

    QFile tagsFile(tags_filename);
    if (tagsFile.exists()) {
        QString bFile = QString(QDir::currentPath() + tBackup + nBackup + "/" + tags_filename);
        if (!tagsFile.copy(bFile)) {
            log->err(QString("Unable to backup TagList %1").arg(bFile));
        }
    }
    else {
        log->err(QString("Could not find file \"%1\" for backup.").arg(tags_filename));
    }

    QFile showsFile(shows_filename);
    if (showsFile.exists()) {
        QString bFile =QString(QDir::currentPath() + tBackup + nBackup + "/" + shows_filename);
        if (!showsFile.copy(bFile)) {
            log->err(QString("Unable to backup ShowList %1").arg(bFile));
        }
    }
    else {
        log->err(QString("Could not find file \"%1\" for backup.").arg(shows_filename));
    }

}

bool ClipDatabase::loadClips(QString clipList_filename) {
    bool importSuccess_flag = true;

    if (!clipList_filename.isEmpty()) {
        QFile clipsFile(clipList_filename);
        if (clipsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream nStream(&clipsFile);

            int cLineNum = 0;
            bool withinList_flag = false;
            QString cListName = "";
            int numAddedtoList = 0;


            while(!nStream.atEnd()) {
                QString line = nStream.readLine();
                cLineNum++;

                if (!line.startsWith("#") && !line.isEmpty()) {
                    bool lineParsed_flag = false;
                    if (!withinList_flag) {
                        if (line.startsWith("List::")) {
                            cListName = line.right(line.length() - 6);
                            numAddedtoList = 0;
                            withinList_flag = true;
                            lineParsed_flag = true;
                        }
                    }
                    else {

                        if (line.startsWith("List::")) {
                            log->err(QString("Already within list %1. Invalid entry \"%2\"").arg(cListName).arg(line));
                            lineParsed_flag = true;
                        }

                        if (line.startsWith("}")) {
                            log->info(QString("Loaded %1 clips to list %2").arg(numAddedtoList).arg(cListName));
                            cListName = "";
                            withinList_flag = false;
                            lineParsed_flag = true;
                        }

                        if (line.startsWith("{")) {
                            lineParsed_flag = true;
                        }
                    }

                    if (!lineParsed_flag) {
                        QVector<QString> nLists;
                        nLists.append(cListName);
                        if (addNewClip(line, nLists) != NULL) {
                            numAddedtoList++;
                        }
                    }

                }
            }

            log->info(QString("Added %1 new Clips.").arg(main_list->getClipCount()));
        }
        else {
            log->warn(QString("Unable to open file \"%1\".").arg(clipList_filename));
            importSuccess_flag = false;
        }
    }
    else {
        log->warn(QString("ClipDatabase.loadClips: Filename is empty"));
        importSuccess_flag = false;
    }

    return importSuccess_flag;
}

bool ClipDatabase::loadShowList(QString showList_filename) {
    bool importSuccess_flag = false;

    if (showList_filename.endsWith(".xml")) {
        log->info(QString("Attempting to load shows from MAL xml file %1.").arg(showList_filename));
        int startCount = existingShows.count();

        QFile nFile(showList_filename);
        if (nFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            importSuccess_flag = true;
            QTextStream nStream(&nFile);

            while (!nStream.atEnd()) {

                QString line = nStream.readLine();
                if (!line.isEmpty() && line.contains(QString("<series_title>"))) {
                    int startBound = line.indexOf("<![CDATA[");
                    int boundLength = 9;
                    int endBound = line.indexOf("]]></series_title");

                    QString title = line.mid(startBound + boundLength, endBound - (startBound + boundLength));
                    existingShows.append(title);
                }
            }
        }

        existingShows.removeDuplicates();
        int diff = existingShows.count() - startCount;
        log->info(QString("Added %1 new shows.").arg(diff));
    }
    else if (showList_filename.endsWith(".txt")) {
        log->info(QString("Attempting to load shows from txt file %1").arg(showList_filename));
        int startCount = existingShows.count();

        QFile showFile(showList_filename);
        if (showFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            importSuccess_flag = true;

            QTextStream tStream(&showFile);

            while(!tStream.atEnd()) {
                QString line = tStream.readLine();

                if (!line.isEmpty() && !line.startsWith('#')) {
                    existingShows.append(line.trimmed());
                }
            }
        }

        existingShows.removeDuplicates();
        int diff = existingShows.count() - startCount;
        log->info(QString("Added %1 new shows.").arg(diff));
    }
    else {
        log->warn(QString("Unrecognized Show file type in file %1").arg(showList_filename));
    }


    return importSuccess_flag;
}

bool ClipDatabase::loadTagList(QString tagList_filename) {
    bool importSuccess_flag = true;

    if (tagManager != NULL) {
        if (!tagList_filename.isEmpty()) {
            QFile tagFile(tagList_filename);
            if (tagFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                log->info(QString("Opened %1 for tag import.").arg(tagList_filename));

                QTextStream tStream(&tagFile);

                while(!tStream.atEnd()) {
                    QString line = tStream.readLine();

                    if (!line.isEmpty() && !line.startsWith('#')) {
                        tagManager->readTagLine(line);
                    }
                }

            }
            else {
                log->warn(QString("ClipDatabase.loadTagList: Unable to open file \"%1\" for tag import.").arg(tagList_filename));
                importSuccess_flag = false;
            }
        }
        else {
            log->warn(QString("ClipDatabase.loadTagList: No TagList in config file"));
            importSuccess_flag = false;
        }

        log->info("ClipDatabase.loadTagList:: Sorting results.");
        tagManager->sortThis();
    }
    else{
        log->err(QString("ClipDatabase.loadTagList: TagManager is NULL"));
        importSuccess_flag = false;
    }



    return importSuccess_flag;
}

Clip* ClipDatabase::addNewClip(QString showName, int epNum, TimeBound time, QVector<QString> nLists) {

    int numListsAdded = 0;
    int numClipsAdded = 0;
    bool clipAdded_flag = true;

    Clip* rClip = clipExists(showName, epNum, time);

    if (rClip == NULL) {
        clipAdded_flag = false;
        rClip = new Clip(log, this);

        rClip->setShowName(showName);
        rClip->setEpNum(epNum);
        rClip->setTimeBound(time);

        if (!existingShows.contains(showName)) {
            existingShows.append(showName);
        }

        if (main_list->addClip(rClip)) {

            clipAdded_flag = true;
            //Add to composite list of clips
            used_clips.append(rClip);

        }
        else {
            log->warn("Clip already exists.");

            delete rClip;
            rClip = NULL;

        }
    }

    if (clipAdded_flag) {

        QVector<ClipList*> remainingLists = sub_lists;

        QMutableVectorIterator<ClipList*> list(remainingLists);

        nLists.removeAll(main_list->getName());

        while(list.hasNext()) {
            list.next();
            QMutableVectorIterator<QString> tLists(nLists);
            while (tLists.hasNext()) {
                QString tListName = tLists.next();
                if (tListName == list.value()->getName()) {
                    if (list.value()->addClip(rClip)) {
                        numListsAdded++;
                    }
                    tLists.remove();
                }
            }

        }

        if (!nLists.empty()) {
            for (int i = 0; i < nLists.count(); i++) {
                if (!nLists.at(i).isEmpty()) {
                    ClipList* nList = new ClipList(log, this);
                    nList->setName(nLists.at(i));
                    sub_lists.append(nList);

                    QString test = QString("Created new list \"%1\".").arg(nList->getName());
                    log->info(test);
                    nList->addClip(rClip);
                    numListsAdded++;
                }
            }
        }

        //log->info(QString("Added Clip to %1 additional lists.").arg(numListsAdded));
    }

    return rClip;
}

Clip* ClipDatabase::addNewClip(QString clipLine, QVector<QString> nLists) {

    Clip *rClip = NULL;
    QString tLine = clipLine.trimmed();

    QStringList lineSplit = tLine.split("[|]");

    if (lineSplit.count() == 9) {
        bool lineValid_flag = true;

        QString nShowName = lineSplit.at(0);
        int nEpNum = lineSplit.at(1).toInt();
        TimeBound nTime;

        QStringList timeSplit = lineSplit.at(2).split("-");

        if (timeSplit.count() == 2) {
            nTime.startTime = QTime::fromString(timeSplit.at(0), QString("hh:mm:ss"));
            nTime.endTime = QTime::fromString(timeSplit.at(1), QString("hh:mm:ss"));
        }
        else {
            lineValid_flag = false;
        }

        QString nSeason = lineSplit.at(3);
        QStringList validSeasons;
        validSeasons << "Spring" << "Summer" << "Fall" << "Winter";
        if (!validSeasons.contains(nSeason, Qt::CaseInsensitive)) {
            nSeason = "Spring";
        }

        int nYear = lineSplit.at(4).toInt();

        QStringList tagSplit = lineSplit.at(5).split("|");

        QString nSource = lineSplit.at(6);
        QString nLink = lineSplit.at(7);
        QString nNote = lineSplit.at(8);


        if (lineValid_flag) {
            rClip = addNewClip(nShowName, nEpNum, nTime, nLists);
            rClip->season = nSeason;
            rClip->year = nYear;
            rClip->tags.append(tagSplit);
            rClip->tags.removeDuplicates();

            tagManager->addTags(tagSplit);
            if (!rClip->localSrc.isEmpty() && nSource != rClip->localSrc) {
                log->warn(QString("New Source does not match existing source. Curr=%1 New=%2").arg(rClip->localSrc).arg(nSource));
            }
            else {
                rClip->localSrc = nSource;
            }
            if (!rClip->link.isEmpty() && nLink != rClip->link) {
                log->warn(QString("New link does not match existing link. Curr=%1 New=%2").arg(rClip->link).arg(nLink));
            }
            else {
                rClip->link = nLink;
            }
            if (!rClip->note.isEmpty() && nNote != rClip->note) {
                log->warn(QString("New Note does not match existing note. Curr=%1 New=%2").arg(rClip->note).arg(nNote));
            }
            else {
                rClip->note = nNote;
            }
        }
    }
    return rClip;
}

void  ClipDatabase::addExistingClip(Clip* /*nClip*/, QVector<ClipList*> /*nLists*/) {

}

ClipList* ClipDatabase::initMainList() {
    if (main_list == NULL) {
        main_list = new ClipList(log, this);
        main_list->setName("General");
    }

    return main_list;
}

Clip* ClipDatabase::clipExists(QString tShowName, int tEpNum, TimeBound tTime) {

    bool exitList_flag = false;
    Clip* rClip = NULL;
    // In main list, find show, if show contains clip

    ShowList *cShow = main_list->getShowList(tShowName);

    if (cShow != NULL) {

        for (int j = 0; j < cShow->clips.count() && !exitList_flag; j++) {
            Clip* cClip = cShow->clips.at(j);

            bool epMatch_flag = (cClip->epNum == tEpNum);
            bool startMatch_flag = (cClip->bounds.startTime == tTime.startTime);
            bool endMatch_flag = (cClip->bounds.endTime == tTime.endTime);

            if (epMatch_flag && startMatch_flag && endMatch_flag) {
                exitList_flag = true;
                rClip = cClip;
            }
        }
    }

    return rClip;
}

TagManager* ClipDatabase::getTagManager() {
    return tagManager;
}
