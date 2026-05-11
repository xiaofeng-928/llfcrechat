#include "registerdialog.h"
#include "ui_registerdialog.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include "global.h"
#include "httpmgr.h"

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog), _countdown(5)
{
    ui->setupUi(this);
    QRegularExpression regExp("[a-zA-Z0-9]+$");
    QValidator *validator = new QRegularExpressionValidator(regExp, this);
    ui->user_edit->setValidator(validator);
    ui->err_tip->setProperty("state", "normal");
    repolish(ui->err_tip);
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish, this,
            &RegisterDialog::slot_reg_mod_finish);
    initHttpHandlers();
    ui->err_tip->clear();

    connect(ui->user_edit, &QLineEdit::editingFinished, this, [this](){
        checkUserValid();
    });

    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });

    connect(ui->confirm_edit, &QLineEdit::editingFinished, this, [this](){
        checkConfirmValid();
    });

    // 创建定时器
    _countdown_timer = new QTimer(this);
    // 连接信号和槽
    connect(_countdown_timer, &QTimer::timeout, [this](){
        if(_countdown == 0){
            _countdown_timer->stop();
            emit sigSwitchLogin();
            return;
        }
        _countdown--;
        auto str = QString("注册成功，%1 s后返回登录").arg(_countdown);
        ui->tip_lb->setText(str);
    });
}

RegisterDialog::~RegisterDialog()
{
    qDebug() << "destruct RegDlg";
    delete ui;
}

void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"), false);
        return;
    }

    // 解析 JSON 字符串
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull()){
        showTip(tr("json解析错误"), false);
        return;
    }

    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"), false);
        return;
    }

    //调用对应的逻辑
    auto it = _handlers.find(id);
    if(it != _handlers.end()) {
        it.value()(jsonDoc.object());
    }
}

bool RegisterDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool RegisterDialog::checkPassValid()
{
    auto pass = ui->pass_edit->text();
    auto confirm = ui->confirm_edit->text();

    if(pass.length() < 6 || pass.length() > 15){
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    if(pass != confirm){
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("密码和确认密码不匹配"));
        return false;
    }else{
        DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }
    return true;
}

bool RegisterDialog::checkConfirmValid()
{
    auto pass = ui->pass_edit->text();
    auto confirm = ui->confirm_edit->text();

    if(confirm.length() < 6 || confirm.length() > 15){
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("密码长度应为6~15"));
        return false;
    }

    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    bool match = regExp.match(confirm).hasMatch();
    if(!match){
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("不能包含非法字符"));
        return false;
    }

    DelTipErr(TipErr::TIP_CONFIRM_ERR);

    if(pass != confirm){
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("确认密码和密码不匹配"));
        return false;
    }else{
        DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }
    return true;
}

void RegisterDialog::initHttpHandlers()
{
    //注册注册用户回包逻辑
    _handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("注册失败，用户可能已存在"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("用户注册成功"), true);
        qDebug() << "email is " << email;
        qDebug() << "user uuid is " << jsonObj["uid"].toString();
        ChangeTipPage();
    });
}

void RegisterDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void RegisterDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }

    showTip(_tip_errs.first(), false);
}

void RegisterDialog::ChangeTipPage()
{
    _countdown_timer->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);

    // 启动定时器，设置间隔为1000毫秒（1秒）
    _countdown_timer->start(1000);
}

void RegisterDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state", "normal");
    }else{
        ui->err_tip->setProperty("state", "err");
    }

    ui->err_tip->setText(str);

    repolish(ui->err_tip);
}

void RegisterDialog::on_sure_btn_clicked()
{
    bool valid = checkUserValid();
    if(!valid){
        return;
    }

    valid = checkPassValid();
    if(!valid){
        return;
    }

    valid = checkConfirmValid();
    if(!valid){
        return;
    }

    //发送http请求注册用户（只需要用户名和密码）
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["passwd"] = xorString(ui->pass_edit->text());
    json_obj["confirm"] = xorString(ui->confirm_edit->text());

    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_register"),
                 json_obj, ReqId::ID_REG_USER, Modules::REGISTERMOD);
}

void RegisterDialog::on_return_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}

void RegisterDialog::on_cancel_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}
