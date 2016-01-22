static inline void eeprom_write(unsigned int n, uint8_t b) { eeprom_write_byte((uint8_t *)n,b); }
static inline uint8_t eeprom_read(int n) { return eeprom_read_byte((uint8_t *)n); }


// чтение и запись мелких объектов
void eeprom_read_len(byte *p, uint16_t e, byte l){
    for(;l>0; l--, e++) {
	*p++ = (byte)eeprom_read_byte( (byte *)e );
    }
}

void eeprom_write_len(byte *p, uint16_t e, byte l){
    for(;  l>0; l--, e++)
	eeprom_write_byte( (byte *)e, *p++ );

}

