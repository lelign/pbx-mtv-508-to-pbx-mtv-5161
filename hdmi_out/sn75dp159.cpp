#include <QLoggingCategory>
#include "sn75dp159.h"

static QLoggingCategory category("SN75DP159 HDMI Retimer");

Sn75dp159::Sn75dp159(uint8_t i2c_addr){
    qCDebug(category) << "initialization sn159dp on " << i2c_filename_sn75dp << "addr" << i2c_addr;
    sn75dp_addr = i2c_addr;
    sn75dp_i2c = new I2c(i2c_filename_sn75dp);

    timer_update_sn75dp = new QTimer;
    connect(timer_update_sn75dp, SIGNAL(timeout()), this, SLOT(sn75dp159_device_id_read()));
    timer_update_sn75dp->setInterval(1000);
    timer_update_sn75dp->start();
}

Sn75dp159::~Sn75dp159(){
    delete sn75dp_i2c;
}

void Sn75dp159::sn75dp159_write(uint8_t reg_addr, uint8_t reg_data){
    int ret = sn75dp_i2c->write(sn75dp_addr, reg_addr, reg_data);
    if (ret == -1) qDebug() << "sn75dp159_write i2c error";
}

void Sn75dp159::sn75dp159_read(uint8_t reg_addr, uint8_t *reg_data){
    int ret = (sn75dp_i2c->read(sn75dp_addr, reg_addr));
    if (ret == -1){
        qDebug() << "sn75dp159_read i2c error";
        *reg_data = 0xFF;
    }
    else *reg_data = (uint8_t)ret;
}

void Sn75dp159::sn75dp159_device_id_read(){
    uint8_t reg_00_data, reg_01_data;
    sn75dp159_read(0x00, &reg_00_data);
	sn75dp159_read(0x01, &reg_01_data);
    qDebug() << "sn75dp159_device_id_read" << hex << reg_00_data << hex << reg_01_data;
}

void Sn75dp159::slot_sn75dp159_update(int rate){
    // sn75dp159_device_id_read();
    uint8_t data;
	
	if (rate == RATE_12G_12170) sn75dp159_write(0x0C, 0x01);	//PRE_SEL = Reg0Ch[1:0] = 01 (labeled HDMI_TWPST)
	else sn75dp159_write(0x0C, 0xFC);							//VSWING_DATA & VSWING_CLK to -7% = Reg0Ch[7:2] = 111111
																//PRE_SEL = Reg0Ch[1:0] = 00: (Labeled HDMI_TWPST)
	sn75dp159_read(0x0B, &data);
	data &= (1 << 1);
	// if (data) flag_tdms_clock = 1;
	// else flag_tdms_clock = 0;

	switch (rate){
		case RATE_12G_12170:
		case RATE_6G_12170:
		data |= 0x98;
		break;
		
		case RATE_3G_12170:
		data |= 0x88;
		break;
		
		case RATE_HD_12170:
		data |= 0x80;
		break;
	}
	sn75dp159_write(0x0B, data);	//TX_TERM_CTL = Reg0Bh[4:3] = 11, SLEW_CTL = Reg0Bh[7:6] = 10
	sn75dp159_write(0x0A, 0x3D);	//APPLY_RXTX_CHANGES set to 1 and other default set
}