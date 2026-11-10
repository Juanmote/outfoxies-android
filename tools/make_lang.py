#!/usr/bin/env python3
"""Traduccion al castellano de los textos del juego -> src/core/lang_es.h

Uso: tools/make_lang.py outfxies.zip   (necesita la ROM para comprobar huecos y punteros)

Los textos van en ASCII dentro del programa (ou2_mpru + ou2_mprl, palabras alternas). En pantalla cada byte es
la loseta 0x340 + byte. '_' es el espacio y en las biografias '\\\\' es salto de linea.
Tres sitios:
  1. Biografias de la presentacion (0xAD47C): una sola tira, lineas de 24 (o 25) caracteres. Se cambia en su
     sitio con el mismo numero de lineas y el mismo largo de cada una, para no tocar el ritmo de las teles.
     Los nombres van en letras rojas (byte 1-26 = A-Z, 0xFB ':', 0xEE '.').
  2. Mensajes fijos con cabecera (0x91C64-0x9201C): 7 bytes (direccion en la capa de texto, color...) + texto
     + 0xFF. Si cabe, en su sitio; si no, se copian a la zona libre y se cambian los punteros que los usan.
     La columna se recoloca para que queden centrados (o alineados a la derecha si lo estaban).
  3. Mensajes de Mr. Acme (0x925FC-0x92BC8): texto + 0xFF, con una tabla de punteros en 0x48770-0x488F8.
     Van todos a la zona libre y se cambian los punteros.
Tildes: no hay letras con tilde en la fuente. Se usan bytes que el juego no escribe y nb2.c dibuja ahi las
letras (hechas a partir de la A, E, I, O, U, N, ? y ! de la propia fuente): ver lang_glyphs en nb2.c.
"""
import sys, struct, zipfile, os

FREE = 0x0F2000           # zona libre del programa (0xFF hasta 0x0FEFFF; los parches de 16:9 usan 0x0FF000)
ACC = {'Á': 0x7b, 'É': 0x7c, 'Í': 0x7d, 'Ó': 0x7e, 'Ú': 0x7f, 'Ñ': 0x60, '¿': 0x5b, '¡': 0x5d}
OK = set('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,:;!?\'"-&/()@\\') | set(ACC)
RED = {'MR.ACME': bytes([0x0d, 0x12, 0xee, 0x01, 0x03, 0x0d, 0x05])}

def enc(s):
    out = bytearray()
    i = 0
    while i < len(s):
        if s.startswith('{', i):
            j = s.index('}', i); out += RED[s[i + 1:j]]; i = j + 1; continue
        c = s[i]
        assert c in OK, (c, s)
        out.append(ACC[c] if c in ACC else 0x5f if c == ' ' else ord(c))
        i += 1
    return bytes(out)

def red_name(s):   # nombre en letras rojas
    return bytes(0x5f if c == ' ' else ord(c) - 64 for c in s)

# ---------------------------------------------------------------- 1. biografias
# (nombre rojo, [lineas]); la primera linea va detras del nombre
BIOS = [
    ('JOHN SMITH', ['THE MAN', 'WHO WOULD DO ANYTHING', 'FROM BABYSITTING TO', 'AGITATING A REVOLUTION', 'TO GET MONEY.'],
     None, ['EL HOMBRE', 'CAPAZ DE HACER LO QUE', 'SEA, DE CUIDAR NIÑOS A', 'INCITAR UNA REVOLUCIÓN,', 'CON TAL DE GANAR DINERO.']),
    ('BETTY DOE', ['A SUCCESSFUL', 'INTERNATIONAL BUSINESS-', 'WOMAN WHO DESPERATELY', 'NEEDS CAPITAL TO FUND', 'HER SEVERAL BUSINESSES.',
                   'HER DIVERSE BACKGROUND', 'INCLUDES BEING A FORMER', 'HIGH PAID HIT WOMAN.'],
     None, ['UNA EXITOSA', 'EMPRESARIA INTERNACIONAL', 'QUE NECESITA CAPITAL CON', 'URGENCIA PARA FINANCIAR', 'SUS MUCHOS NEGOCIOS.',
            'SU VARIADO PASADO', 'INCLUYE HABER SIDO UNA', 'SICARIA MUY BIEN PAGADA.']),
    ('BERNARD WHITE', ['A BIO-', 'TECH ENGINEER WITH', 'SUPER HUMAN STRENGTH.', 'AN EXPERIMENT RESULTED', 'IN HIM LOSING HIS HAND.',
                       'HE NOW HAS AN ARTIFICIAL', 'HAND MADE OF STEEL.'],
     None, ['INGENIERO', 'BIOTECNOLÓGICO CON', 'FUERZA SOBREHUMANA.', 'UN EXPERIMENTO LE HIZO', 'PERDER UNA MANO.',
            'AHORA TIENE UNA MANO', 'ARTIFICIAL DE ACERO.']),
    ('EVE', ['A "HAS-BEEN" FILM', 'STARLET, NOW A THIEF', 'TO SUPPORT HER LAVISH', 'LIFESTYLE. SHE HAS THE', 'SKILL TO BREAK INTO NOX',
             'FORTRESS WITH THE AID OF', 'HER WELL-TRAINED LIZARD.'],
     None, ['EX ESTRELLA DE CINE', 'AHORA LADRONA PARA', 'PAGARSE SU VIDA DE LUJO.', 'ES CAPAZ DE ENTRAR EN', 'LA FORTALEZA DE NOX',
            'CON LA AYUDA DE SU', 'LAGARTO BIEN ENTRENADO.']),
    ('PROFESSOR CHING', ['THE', 'MYSTERIOUS ORIENTAL', 'SCIENTIST WHO RIDES', 'ON A KILLER-WHEEL-CHAIR', 'HE DESIGNED.THOUGH',
                         'PARTIALLY HANDICAPPED,', 'HE IS A FAMOUS MASTER OF', 'CHINESE MARTIAL ARTS.'],
     None, ['EL', 'MISTERIOSO CIENTÍFICO', 'ORIENTAL QUE VA EN UNA', 'SILLA DE RUEDAS ASESINA', 'DISEÑADA POR ÉL. AUNQUE',
            'TIENE UNA DISCAPACIDAD,', 'ES UN FAMOSO MAESTRO DE', 'ARTES MARCIALES CHINAS.']),
    ('DWEEB', ['THE ONLY KNOWN', 'PROFESSIONAL KILLER', 'WHO WORKS FOR BANANAS.', 'HE IS THE KIDNAPPED SON', 'OF THE FAMOUS TV STAR,',
               'WORLDS MOST INTELLIGENT', 'CHIMPANZEE "MR.HAPPY".'],
     None, ['EL ÚNICO ASESINO', 'PROFESIONAL CONOCIDO', 'QUE COBRA EN PLÁTANOS.', 'ES EL HIJO SECUESTRADO', 'DEL CHIMPANCÉ MÁS LISTO',
            'DEL MUNDO, ESTRELLA DE', 'LA TELE: "MR.HAPPY".']),
    ('DANNY AND DEMI', ['THE EX-', 'SIAMESE TWINS, SEPARATED', 'BY A TRAIN CRUSH.', 'BESIDES BEING EXTREMELY', 'INTELLIGENT, THEY BOTH',
                        'POSSESS A SINISTER', 'AND CRIMINAL MIND.', ''],
     'DANNY Y DEMI', ['LOS EX-', 'SIAMESES, SEPARADOS EN', 'UN ACCIDENTE DE TREN.', 'ADEMÁS DE SER MUY', 'INTELIGENTES, LOS DOS',
                      'TIENEN UNA MENTE', 'SINIESTRA Y CRIMINAL.', '']),
    (None, ['A MYSTERY MAN NAMED', 'MR.ACME CONTRACTED', 'SEVEN KILLERS', 'TO ASSASSINATE', 'SEVEN WORLD FAMOUS', 'ART COLLECTORS.',
            'ODDLY ENOUGH,', 'EACH KILLER WAS', 'ALSO HIRED', 'TO ELIMINATE', 'THE OTHER SIX KILLERS...', "WHAT IS MR.ACME'S",
            'MOTIVE?', 'WHAT IS IN STORE', 'FOR THE SURVIVOR?', 'IS IT A LARGE REWARD', 'AND THE REPUTATION', 'AS BEING',
            'THE NUMBER ONE', 'KILLER?'],
     None, ['UN HOMBRE MISTERIOSO', 'LLAMADO {MR.ACME} CONTRATÓ', 'A SIETE ASESINOS', 'PARA ACABAR CON SIETE', 'FAMOSOS COLECCIONISTAS',
            'DE ARTE DEL MUNDO.', 'CURIOSAMENTE,', 'A CADA ASESINO', 'TAMBIÉN LE ENCARGÓ', 'ELIMINAR', 'A LOS OTROS SEIS...',
            '¿CUÁL ES EL MÓVIL', 'DE {MR.ACME}?', '¿QUÉ LE ESPERA', 'AL SUPERVIVIENTE?', '¿UNA GRAN RECOMPENSA', 'Y LA FAMA',
            'DE SER', 'EL MEJOR', 'ASESINO DEL MUNDO?']),
]

# ---------------------------------------------------------------- 2. mensajes fijos (direccion de la cabecera)
# alineacion: c centrado, r a la derecha, k columna tal cual
FIXED = [
    (0x91c64, 'c', 'KILL YOUR ENEMY BY ANY MEANS.', '¡ACABA CON TU ENEMIGO COMO SEA!'),
    (0x91c8c, 'c', 'HURRY UP!', '¡DATE PRISA!'),
    (0x91ca0, 'c', 'EACH ARE DEAD...', 'AMBOS MUERTOS...'),
    (0x91cb8, 'c', 'JOHN SMITH WON!', '¡GANA JOHN SMITH!'),
    (0x91cd0, 'c', 'EVE WON!', '¡GANA EVE!'),
    (0x91ce0, 'c', 'BETTY DOE WON!', '¡GANA BETTY DOE!'),
    (0x91cf8, 'c', 'BERNARD WHITE WON!', '¡GANA BERNARD WHITE!'),
    (0x91d14, 'c', 'PROF.CHING WON!', '¡GANA PROF.CHING!'),
    (0x91d2c, 'c', 'DWEEB WON!', '¡GANA DWEEB!'),
    (0x91d40, 'c', 'DANNY & DEMI WON!', '¡GANAN DANNY Y DEMI!'),
    (0x91d5c, 'c', 'MR.ACME WON!', '¡GANA MR.ACME!'),
    (0x91d70, 'c', 'JOHN SMITH SURVIVED.', 'JOHN SMITH SOBREVIVE.'),
    (0x91d8c, 'c', 'EVE SURVIVED.', 'EVE SOBREVIVE.'),
    (0x91da4, 'c', 'BETTY DOE SURVIVED.', 'BETTY DOE SOBREVIVE.'),
    (0x91dc0, 'c', 'BERNARD WHITE SURVIVED.', 'BERNARD WHITE SOBREVIVE.'),
    (0x91de0, 'c', 'PROF.CHING SURVIVED.', 'PROF.CHING SOBREVIVE.'),
    (0x91dfc, 'c', 'DWEEB SURVIVED.', 'DWEEB SOBREVIVE.'),
    (0x91e14, 'c', 'DANNY & DEMI SURVIVED.', 'DANNY Y DEMI SOBREVIVEN.'),
    (0x91e34, 'c', 'MR.ACME SURVIVED.', 'MR.ACME SOBREVIVE.'),
    (0x91e50, 'c', 'SOMEONE IS TAKING AIM ON YOU!', '¡ALGUIEN TE ESTÁ APUNTANDO!'),
    (0x91e78, 'c', 'CONTINUE?', '¿CONTINUAR?'),
    (0x91e8c, 'c', 'GAME  OVER', 'FIN  DE  LA  PARTIDA'),
    (0x91ed0, 'c', 'CONTINUE?', '¿CONTINUAR?'),
    (0x91ee4, 'c', 'PLEASE WAIT!', '¡UN MOMENTO!'),
    (0x91f48, 'k', 'SELECT YOUR CHARACTER\\\\  WITHIN    SECONDS.', ' ELIGE TU PERSONAJE  \\\\  TIENES    SEGUNDOS.'),
    (0x91f7c, 'k', 'SECOND. ', 'SEGUNDO. '),
    (0x91f8c, 'c', 'THE BEST 5 OUTFOXIES', 'LOS 5 MEJORES OUTFOXIES'),
    (0x91fa8, 'r', '       REWARD', 'RECOMPENSA'),
    (0x91fc0, 'r', 'HOSPITAL BILL', 'HOSPITAL'),
    (0x91fd8, 'r', '       PROFIT', 'BENEFICIO'),
    (0x91ff0, 'r', ' BANK BALANCE', 'SALDO BANCARIO'),
    (0x92008, 'k', 'UNLIMITED', 'ILIMITADA'),
    (0x9201c, 'k', 'UNLIMITED', 'ILIMITADA'),
]
FIXED_KEEP = ['WIN / WINS (marcador de victorias: falta ver dónde sale)', '(c)1994 NAMCO LTD. / ALL RIGHTS RESERVED / TM (aviso legal)',
              'Créditos finales (nombres del equipo de Namco)']

# ---------------------------------------------------------------- 3. mensajes de Mr. Acme (direccion original)
ACME = [
    (0x925fc, 'INSTRUCTIONS FROM MR.ACME', 'INSTRUCCIONES DE MR.ACME'),
    (0x92616, 'YOU DID A GREAT JOB,', 'BUEN TRABAJO,'),
    (0x9262b, 'YOUR NEXT TARGET IS', 'TU PRÓXIMO OBJETIVO ES'),
    (0x9263f, 'YOU ARE BOTH', 'LOS DOS SOIS'),
    (0x9264c, 'PROFESSIONAL KILLERS,', 'ASESINOS PROFESIONALES.'),
    (0x92662, 'PROCEED WITH CAUTION!', '¡VE CON CUIDADO!'),
    (0x92678, 'IF ANITHING HAPPENS, MAKE', 'SI PASA ALGO, ASEGÚRATE'),
    (0x92692, 'SURE NOBODY FINDS OUT', 'DE QUE NADIE DESCUBRA'),
    (0x926a8, 'THAT WE KNOW EACH OTHER.', 'QUE NOS CONOCEMOS.'),
    (0x926c1, 'YOUR MISSION IS TO MARK', 'VUESTRA MISIÓN ES'),
    (0x926d9, 'EACH OTHER AS TARGETS.', 'IR EL UNO A POR EL OTRO.'),
    (0x926f0, 'IT IS A BATTLE OF', 'ES UNA LUCHA POR'),
    (0x92702, 'SURVIVAL.', 'SOBREVIVIR.'),
    (0x9270c, 'THE REWARD TO THE WINNER', 'LA RECOMPENSA PARA'),
    (0x92725, 'WILL BE...', 'EL GANADOR SERÁ...'),
    (0x92730, '3000000 SWISS FRANCS.', '3000000 FRANCOS SUIZOS.'),
    (0x92746, 'IN MY OPINYON YOU TWO ARE', 'EN MI OPINIÓN, ESTÁIS'),
    (0x92760, 'EVENRY MATCHED.', 'MUY IGUALADOS.'),
    (0x92770, 'NOW OUR CONTRACT IS', 'NUESTRO CONTRATO'),
    (0x92784, 'COMPLETED...', 'HA TERMINADO...'),
    (0x92791, 'LET US PLAN ON MEETING', 'VEÁMONOS EN'),
    (0x927a8, 'AT MY HOUSE.', 'MI CASA.'),
    (0x927b5, 'I WILL PAY YOU THERE.', 'ALLÍ TE PAGARÉ.'),
    (0x927cb, 'WELCOME,', 'TE DOY LA BIENVENIDA.'),
    (0x927d4, "I AM ACME'S WIFE.", 'SOY LA MUJER DE ACME.'),
    (0x927e6, 'AS WE PROMISSED, WE SHALL', 'COMO PROMETIMOS,'),
    (0x92800, 'PAY YOU RIGHT NOW.', 'TE PAGAREMOS AHORA MISMO.'),
    (0x92813, 'I OFFER YOU 800000 SWISS', 'TE PAGO 800000 FRANCOS'),
    (0x9282c, 'I OFFER YOU 1200000 SWISS', 'TE PAGO 1200000 FRANCOS'),
    (0x92846, 'I OFFER YOU 1600000 SWISS', 'TE PAGO 1600000 FRANCOS'),
    (0x92860, 'I OFFER YOU 2000000 SWISS', 'TE PAGO 2000000 FRANCOS'),
    (0x9287a, 'I OFFER YOU 2400000 SWISS', 'TE PAGO 2400000 FRANCOS'),
    (0x92894, 'I OFFER YOU 2800000 SWISS', 'TE PAGO 2800000 FRANCOS'),
    (0x928ae, 'I OFFER YOU 3200000 SWISS', 'TE PAGO 3200000 FRANCOS'),
    (0x928c8, 'FRANCS TO ELIMINATE HIM.', 'SUIZOS POR ELIMINARLO.'),
    (0x928e1, 'FRANCS TO ELIMINATE HER.', 'SUIZOS POR ELIMINARLA.'),
    (0x928fa, 'FRANCS TO ELIMINATE THEM.', 'SUIZOS POR ELIMINARLOS.'),
    (0x92914, 'YOU WILL FIND HIM', 'LO ENCONTRARÁS'),
    (0x92926, 'YOU WILL FIND HER', 'LA ENCONTRARÁS'),
    (0x92938, 'YOU WILL FIND THEM', 'LOS ENCONTRARÁS'),
    (0x9294b, 'MR.SMITH.', 'SR.SMITH.'),
    (0x92955, 'EVE.', 'EVE.'),
    (0x9295a, 'MS.DOE.', 'SRTA.DOE.'),
    (0x92962, 'MR.WHITE.', 'SR.WHITE.'),
    (0x9296c, 'PROFESSOR CHING.', 'PROFESOR CHING.'),
    (0x9297d, 'DWEEB.', 'DWEEB.'),
    (0x92984, 'DANNY, DEMI.', 'DANNY, DEMI.'),
    (0x92991, 'JOHN SMITH.', 'JOHN SMITH.'),
    (0x9299d, 'BETTY DOE.', 'BETTY DOE.'),
    (0x929a8, 'BERNARD WHITE.', 'BERNARD WHITE.'),
    (0x929b7, 'DANNY AND DEMI.', 'DANNY Y DEMI.'),
    (0x92a0d, 'IN A DOWNTOWN SKYSCRAPER', 'EN UN RASCACIELOS DEL'),
    (0x92a26, 'WHERE A WIRED EXPLOSIVE', 'CENTRO, DONDE HAN PUESTO'),
    (0x92a3e, 'HAS BEEN SET.', 'UNA BOMBA.'),
    (0x92a4c, 'AT THE CIRCUS,', 'EN EL CIRCO,'),
    (0x92a5b, 'SHE WILL MAKE', 'DONDE ACTUARÁ'),
    (0x92a69, 'HER APPEARANCE TONIGHT.', 'ESTA NOCHE.'),
    (0x92a81, 'AT THE SEASIDE AQURIUM,', 'EN EL ACUARIO MARINO,'),
    (0x92a99, 'WHERE SHE HAVE PLANTED', 'DONDE HA COLOCADO'),
    (0x92ab0, 'A WIRED EXPLOSIVE.', 'UNA BOMBA.'),
    (0x92ac3, 'ON THE TRAIN GOING ACROSS', 'EN EL TREN QUE CRUZA'),
    (0x92add, 'THE ROCKY MOUNTAINS.', 'LAS MONTAÑAS ROCOSAS.'),
    (0x92af3, 'AT THE WEAPONS FACTORY,', 'EN LA FÁBRICA DE ARMAS,'),
    (0x92b0b, 'HE IS WORKING AS A SECRET', 'DONDE TRABAJA EN SECRETO'),
    (0x92b25, 'ADVISOR FOR WEAPONS.', 'COMO ASESOR DE ARMAMENTO.'),
    (0x92b3a, 'ON A PLANE FOR THE SOUTH.', 'EN UN AVIÓN HACIA EL SUR.'),
    (0x92b54, 'HE PLANS ON STEALING', 'ALLÍ PIENSA ROBAR'),
    (0x92b69, 'VALUABLE MUNITIONS THERE.', 'UNA VALIOSA MUNICIÓN.'),
    (0x92b83, 'ON A CRUISE SHIP LEAVING', 'EN UN CRUCERO QUE ZARPA'),
    (0x92b9c, 'THIS EVENING. THEY', 'ESTA TARDE. QUIEREN'),
    (0x92baf, 'HOPE TO ESCAPE TO EUROPE.', 'HUIR A EUROPA.'),
]
ACME_MAX = 25


def load_prog(zpath):
    z = zipfile.ZipFile(zpath)
    u, l = z.read('ou2_mpru.11d'), z.read('ou2_mprl.11c')
    p = bytearray(2 * len(u))
    for i in range(0, len(u), 2):
        p[i * 2:i * 2 + 2] = u[i:i + 2]; p[i * 2 + 2:i * 2 + 4] = l[i:i + 2]
    return bytes(p)


def build(p):
    writes = []          # (direccion, bytes)
    free = FREE

    def put_free(b):
        nonlocal free
        a = free; writes.append((a, b)); free += (len(b) + 1) & ~1
        return a

    def refs(a):
        t = struct.pack('>I', a)
        return [i for i in range(0, 0xF0000, 2) if p[i:i + 4] == t]

    # 1. biografias
    st = p.index(bytes([0x0a, 0x0f, 0x08, 0x0e, 0x5f, 0x13]), 0xad400)
    end = p.index(b'\xff', st)
    lines = p[st:end].split(b'\\\\')
    out, li = [], 0
    for name, en, newname, es in BIOS:
        assert len(en) == len(es), name
        for k, t in enumerate(es):
            orig = lines[li]; w = len(orig)
            if k == 0 and name:
                cut = orig.index(0xfb) + 2                       # nombre rojo + ':' + espacio
                head = (red_name(newname) + b'\xfb_') if newname else orig[:cut]
                body = enc(t)
            else:
                head, body = b'', enc(t)
            assert len(head) + len(body) <= w, (name, t, len(head) + len(body), w)
            out.append(head + body + b'_' * (w - len(head) - len(body)))
            li += 1
    out += lines[li:]
    new = b'\\\\'.join(out)
    assert len(new) == end - st, (len(new), end - st)
    writes.append((st, new))

    # 2. mensajes fijos
    for a, al, en, es in FIXED:
        assert p[a] == 0 and p[a + 1] == 0x68, hex(a)
        hdr = bytearray(p[a:a + 7]); oe = p.index(b'\xff', a + 7)
        old = p[a + 7:oe]
        assert old.replace(b'_', b' ') == en.replace(' ', ' ').encode(), (hex(a), old, en)
        b = enc(es)
        slot = 7 + len(old)
        while p[a + slot] == 0xff: slot += 1
        addr = struct.unpack('>H', hdr[2:4])[0]
        d = 0
        if al == 'c': d = (len(old) - len(b)) // 2
        elif al == 'r': d = len(old) - len(b)
        addr += 2 * d
        hdr[2:4] = struct.pack('>H', addr)
        rec = bytes(hdr) + b + b'\xff'
        if len(rec) <= slot:
            writes.append((a, rec + b'\xff' * (slot - len(rec))))
        else:
            na = put_free(rec)
            rs = refs(a); assert rs, hex(a)
            for r in rs: writes.append((r, struct.pack('>I', na)))

    # 3. Mr. Acme
    for a, en, es in ACME:
        e = p.index(b'\xff', a)
        assert p[a:e].replace(b'_', b' ') == en.encode(), (hex(a), p[a:e], en)
        b = enc(es); assert len(b) <= ACME_MAX, (es, len(b))
        na = put_free(b + b'\xff')
        rs = [r for r in refs(a) if 0x48700 <= r < 0x48900]; assert rs, hex(a)
        for r in rs: writes.append((r, struct.pack('>I', na)))
    assert free < 0x0FF000
    return writes


def main():
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    p = load_prog(sys.argv[1])
    writes = build(p)
    o = ['/* Generado por tools/make_lang.py (traduccion al castellano): no editar */',
         '#ifndef OFX_LANG_ES_H', '#define OFX_LANG_ES_H',
         'static const struct { unsigned addr, len; const unsigned char *b; } lang_es[] = {']
    for a, b in writes:
        o.append('    {0x%06x, %d, (const unsigned char[]){%s}},' % (a, len(b), ','.join('0x%02x' % x for x in b)))
    o += ['};', '#endif']
    open(os.path.join(root, 'src/core/lang_es.h'), 'w').write('\n'.join(o) + '\n')
    print(len(writes), 'escrituras, %d bytes' % sum(len(b) for _, b in writes))


if __name__ == '__main__':
    main()
