# Textos del manual (tools/make_manual.py) en castellano, ingles, portugues (de Portugal, como el menu) e italiano.
# Los numeros de las fichas (velocidades, saltos, punteria...) estan en make_manual.py; aqui solo va el texto.
# Fuentes: biografias del juego, datos de la ROM (documento "Personajes"), Wikipedia en castellano y lo jugado por el
# usuario (curiosidades de cada personaje: 17-11-2026).

LANGS = ['es', 'en', 'pt', 'it']
SUFIJO = {'es': 'Castellano', 'en': 'English', 'pt': 'Portugues', 'it': 'Italiano'}
DEC = {'es': ',', 'en': '.', 'pt': ',', 'it': ','}

T = {}

# ======================================================================== CASTELLANO
T['es'] = dict(
    cover_title='MANUAL DEL JUEGO',
    cover_sub='Historia  ·  Personajes  ·  Armas  ·  Escenarios  ·  Finales',
    sec_story='Historia principal', story_title='SIETE ASESINOS, UN SOLO SUPERVIVIENTE', story_panel='El encargo de Mr. Acme',
    story=['Un hombre misterioso llamado Mr. Acme contrató a siete asesinos para acabar con siete famosos coleccionistas de arte '
           'del mundo. Curiosamente, a cada asesino también le encargó eliminar a los otros seis...',
           '¿Cuál es el móvil de Mr. Acme? ¿Qué le espera al superviviente? ¿Una gran recompensa y la fama de ser el mejor asesino '
           'del mundo?',
           'Eliges a uno de los siete. Antes de cada combate, Mr. Acme te da las instrucciones por la tele: quién es tu próximo '
           'objetivo, cuánto te paga y dónde encontrarlo. Cada combate se libra en el terreno del objetivo, y la única regla es la '
           'del cartel que lo abre: ¡acaba con tu enemigo como sea!',
           'Cuando hayas eliminado a los otros seis, queda el último encargo: llegar hasta la mansión del propio Mr. Acme.'],
    howto_title='CÓMO SE JUEGA', rules_panel='Reglas del combate',
    rules=['Todos empiezan con la barra de energía llena. Nadie tiene más vida que otro: lo que cambia es cuánto le quita cada golpe.',
           '200 segundos por combate. Si se acaba el tiempo, pierde el jugador con menos vida.',
           'Las armas van apareciendo por el escenario: pistola, metralleta, lanzacohetes, granada, katana, látigo y lanzallamas. '
           'También se puede lanzar lo que haya a mano: sopa, tarta, cesta de fruta o carbón.',
           'Tras recibir un golpe, el luchador parpadea un segundo y no le pueden volver a dar.',
           'Con dos toques seguidos hacia un lado se corre; corriendo se salta más lejos, se desliza agachándose y se hace un placaje.',
           'El escenario también ataca: caer al vacío quita energía y hay trampas propias de cada sitio.'],
    dmg_panel='Daño de cada ataque', dmg_cols=('ATAQUE', 'BASE', 'BARRA DE JOHN'),
    dmg_rows=['Cohete / explosión de granada', 'Caer al vacío', 'Pistola (por bala)', 'Katana (por golpe)', 'Lanzallamas (por toque)',
              'Sopa', 'Látigo (por golpe)', 'Metralleta (por bala)', 'Puñetazo (Bernard: 3)', 'Tarta'],
    dmg_note='Daño base de la ROM. El porcentaje es lo que le quita a John Smith en Normal; a los demás, según lo que aguante cada '
             'uno (ver las fichas).',
    sec_chars='Fichas de los personajes',
    p_data='Datos', p_who='Quién es', stage_lbl='Escenario: ', p_stats='Estadísticas', p_arms='Armas y golpes', p_pros='Pros',
    p_cons='Contras', p_cur='Curiosidades',
    st=('Andar', 'Correr', 'Salto', 'Aguante', 'Puntería'), st_recv='recibe %d %%', st_gun='pistola %d°',
    arms=('Abanico con pistola', 'Con lanzacohetes', 'Con metralleta', 'Placaje a la carrera', 'Puñetazo'),
    cmp_title='COMPARATIVA', cmp_panel='Los siete, frente a frente',
    cmp_cols=['Personaje', 'Andar', 'Correr', 'Salto', 'Recibe', 'Pistola', 'Cohetes', 'Metralleta', 'Placaje'],
    cmp_note='En verde, el mejor de cada columna. Andar y correr en píxeles por fotograma; salto, altura en píxeles; recibe, daño '
             'comparado con Bernard (100 %); armas, abanico de desvío de cada disparo (menos = más recto).',
    acme_panel='El que lo empezó todo',
    acme=['El misterioso cliente que contrató a los siete asesinos y que, por lo que se ve, quería que solo quedara uno. Habla '
          'siempre a través de la tele, con las instrucciones de cada encargo.',
          'Al final espera en su mansión junto a Mrs. Acme. Fuera, ataca desde su helicóptero; para derribarlo hay un lanzamisiles '
          'especial (SPECIAL ROCKET) con munición ILIMITADA.',
          'Su mansión está hecha para pelear contra la máquina: por eso no sale en el combate entre dos jugadores.'],
    sec_weap='Armas', weap_title='EL ARSENAL', ammo_lbl='Munición: ', dmg_lbl='Daño: ',
    weap=[('PISTOLA', '7 balas', '20', 'La más común. ¡Cuidado con quedarte sin munición! Betty es la que más recto la dispara.'),
          ('METRALLETA', '80 balas', '2 por bala', 'Poco daño, pero dispara rápido. Tras cada impacto el rival es inmune un segundo.'),
          ('LANZACOHETES', '3 cohetes', '35', 'Lo que más quita. ¡Cuidado con el retroceso! Ching lo dispara perfectamente recto.'),
          ('LANZALLAMAS', '300 de gas', '10 por toque', '¡Lo reduce todo a cenizas! Tras cada toque el rival es inmune un segundo: '
           'no suma daño seguido.'),
          ('GRANADA', '1', '35 (explosión)', 'Quita el pasador y lánzala. Si te la lanzan, ¡puedes devolverla! El lagarto de Eve '
           'también sabe.'),
          ('KATANA', 'sin límite', '15', 'Si mantienes apretado el botón de ataque, la usarás para cubrirte de las balas, incluso '
           'pudiendo correr con ella así. Pero cuidado, la metralleta puede romperte la katana.'),
          ('LÁTIGO', 'sin límite', '5', 'Ataca y derriba a los rivales en el momento justo.')],
    use_panel='Cómo se usan',
    use=['Agáchate sobre un arma para recogerla.', 'Solo se lleva una (los gemelos, una de reserva).',
         'Sin munición, el arma se acaba perdiendo.',
         'Las balas pueden chocar entre sí, e incluso puedes detonar un misil si le aciertas con una bala.'],
    obj_title='OBJETOS Y TRAMPAS', throw_panel='Todo lo que se puede lanzar',
    throw='Acércate a cajas, bidones y demás, y mantén pulsado el botón de ATAQUE para cogerlos y llevarlos; suéltalos para '
          'lanzarlos. ¡Puede haber objetos inesperados... utilízalos con cabeza!',
    throw_rows=[('Sopa', '10'), ('Granada (si golpea sin explotar)', '1'), ('Carbón (tren)', '1'), ('Fruta de la cesta (6 piezas)', '0 a 2'),
                ('Tarta', '0')],
    traps_panel='Armas y trampas de los escenarios',
    traps=['Avión: metralletas montadas en el propio avión.', 'Circo: los payasos dejan pistolas al dispararles, algunas de juguete.',
           'Fábrica: los ascensores suben lanzacohetes.', 'Tren: dentro del vagón del medio hay objetos muy calientes.',
           'Mansión: lanzamisiles especial con munición ILIMITADA para el helicóptero de Mr. Acme.',
           'Caer al vacío o al agua quita energía (25 de base).'],
    sec_stages='Escenarios', turf='Terreno de ',
    stages=[('RASCACIELOS', 'John Smith', 'Un edificio de oficinas de arriba abajo. El combate empieza en la azotea, junto a un '
             'helicóptero; hacia la mitad del tiempo el helicóptero explota y todos caen al comedor de la planta de abajo. En el '
             'sótano, a la izquierda del garaje, hay un foso con siete granadas en fila.'),
            ('CIRCO', 'Eve', 'La carpa de un circo, con trapecios y cuerdas por encima de la pista y el público alrededor. Los '
             'trapecios sirven para cruzar de lado a lado. Según pasa el tiempo salen payasos a los que, si disparas, dejarán '
             'pistolas (puede que falsas), así como un tigre, un elefante y el cañón del hombre bala.'),
            ('ACUARIO', 'Betty Doe', 'Un acuario con una ballena en su tanque, peces y varias plantas unidas por escaleras. Con el '
             'tiempo explotará una bomba en la sujeción de la ballena, que rompe las columnas del acuario y lo va llenando de agua '
             'y de todas las criaturas marinas que estaban encerradas.'),
            ('TREN', 'Bernard White', 'Se pelea encima de un tren de vapor en marcha, entre montañas: vagones, ténder con carbón '
             'para lanzar y la locomotora. Quien cae del tren por delante o por detrás vuelve a subir en una vagoneta. Si rompes '
             'las paredes del vagón del medio, hay objetos muy calientes.'),
            ('FÁBRICA', 'Prof. Ching', 'La fábrica del profesor: un laberinto de pasarelas, escaleras, cintas y ascensores que suben '
             'cohetes. Es de los escenarios más grandes, y la cámara se aleja mucho para enseñarlo entero. El paraíso de los '
             'cohetes y las balas.'),
            ('AVIÓN', 'Dweeb', 'Un avión en pleno vuelo, por fuera y por dentro, con un helicóptero alrededor. Tiene metralletas '
             'montadas, así que aprovéchalas. Si alguien cae al vacío, vuelve con menos energía.'),
            ('BARCO', 'Danny y Demi', 'Un yate en alta mar con varias cubiertas, del puente a las bodegas. Por los lados está el mar: '
             'quien cae al agua acaba volviendo a bordo, eso sí, si no le comen antes. El mayor reto de este nivel es conseguir apuntar.'),
            ('MANSIÓN', 'Mr. Acme', 'El último encargo: la mansión de Mr. Acme, por dentro y por fuera. Dentro hay perros, animales '
             'acuáticos, trampas y explosiones para conseguir que no llegues a la confrontación final con Mr. Acme.')],
    sec_end='Finales', end_title='EL SUPERVIVIENTE', end_panel='Al vencer a Mr. Acme',
    end='Si acabas con Mr. Acme y con su jefe (su esposa), pasarás a ver el final de tu personaje con el resultado económico de tu '
        'trabajo matando a tus rivales.',
    end_rows=[('RECOMPENSA', 'lo que paga Mr. Acme por los encargos'), ('HOSPITAL', 'la factura de tus heridas'),
              ('BENEFICIO', 'lo que te queda limpio'), ('SALDO BANCARIO', 'y, por último, tu cuenta')],
    end_tag='¡Cuanto menos te hayan herido, más dinero!', end_cap='Las cuentas de Eve tras acabar con Mr. Acme',
    pending='FOTO PENDIENTE',
    pages=dict(cover='Portada', story='Historia', howto='Como se juega', card='Ficha %s', cmp='Comparativa', acme='Mr. Acme',
               weap='Armas', obj='Objetos', stage='Escenario %s', end='Finales', fin='Final %s'),
    pdf='The Outfoxies Remastered - Manual.pdf',
    ch={
        'john': dict(name='JOHN SMITH', esc='Rascacielos', tcor='0,4 s',
            bio='El hombre capaz de hacer lo que sea, de cuidar niños a incitar una revolución, con tal de ganar dinero.',
            pros=['El más equilibrado: ni débil ni lento en nada.', 'El segundo que menos daño recibe.', 'Buena puntería con todas las armas.'],
            cons=['No destaca en nada concreto.', 'Corriendo es de los normales (4,0).'],
            cur=['Hijo de un oficial de las fuerzas de ocupación japonesas, aprendió los secretos del zen.',
                 'Es tuerto del ojo izquierdo.'],
            wiki=[('Edad', '38 años (1956)'), ('Altura', '1,80 m'), ('Peso', '80 kg'), ('Origen', 'EE. UU. (nació en Hiroshima)'),
                  ('Oficio', 'Mercenario'), ('Remate', 'Pega en la tele una etiqueta de "HECHO"')]),
        'eve': dict(name='EVE', esc='Circo', tcor='0,5 s',
            bio='Ex estrella de cine, ahora ladrona para pagarse su vida de lujo. Es capaz de entrar en la fortaleza de Nox con la '
                'ayuda de su lagarto bien entrenado.',
            pros=['La más rápida andando de todo el juego.', 'Salta más alto que nadie: llega a plataformas que otros no alcanzan.',
                  'Aguanta bien para ser tan ágil.'],
            cons=['Puntería algo peor que John.', 'Su placaje es de los flojos (4).'],
            cur=['El lagarto que lleva a la espalda hace de barrera y bloquea las balas enemigas, aunque solo una vez.',
                 'Tras bloquearlas, cae y se mueve por su cuenta, encendiendo las granadas que caen y llevándoselas.'],
            wiki=[('Nombre', 'Lisa "Eve" Price'), ('Edad', '29 años (1965)'), ('Altura', '1,65 m · 50 kg'), ('Origen', 'Londres (Reino Unido)'),
                  ('Oficio', 'Ladrona y asesina; antes, actriz'), ('Remate', 'Dispara a la tele')]),
        'betty': dict(name='BETTY DOE', esc='Acuario', tcor='al instante',
            bio='Una exitosa empresaria internacional que necesita capital con urgencia para financiar sus muchos negocios. Su '
                'variado pasado incluye haber sido una sicaria muy bien pagada.',
            pros=['La que dispara más recto con cualquier arma (pistola 4°).', 'Llega a su velocidad de carrera al instante.'],
            cons=['Corriendo es la más lenta.', 'Salta poco y recibe un 17 % más que Bernard.'],
            cur=['Una de las «empresarias más exitosas del siglo XX».',
                 'Pasó de nacer en un barrio pobre a tener el tercer ingreso personal más alto de EE. UU.'],
            wiki=[('Edad', '33 años (1961)'), ('Altura', '1,57 m'), ('Peso', '43 kg'), ('Origen', 'Los Ángeles (EE. UU.)'),
                  ('Oficio', 'Empresaria hecha a sí misma'), ('Remate', 'Apaga la tele')]),
        'bernard': dict(name='BERNARD WHITE', esc='Tren', tcor='1,2 s',
            bio='Ingeniero biotecnológico con fuerza sobrehumana. Un experimento le hizo perder una mano: ahora tiene una mano '
                'artificial de acero.',
            pros=['El que menos daño recibe de todos.', 'Su puñetazo pega el triple que el de los demás.', 'El placaje más fuerte (12).',
                  'El segundo más rápido andando.'],
            cons=['Salta poco (55).', 'Es el que más tarda en arrancar a correr.', 'Su pistola se desvía hacia un lado.'],
            cur=['Odia matar animales, pero no siente ningún remordimiento por matar personas.',
                 'Está batiendo el récord mundial de asesinatos en masa con sus propias manos.'],
            wiki=[('Edad', '36 años (1958)'), ('Altura', '2,16 m'), ('Peso', '135 kg'), ('Origen', 'Washington D. C. (EE. UU.)'),
                  ('Oficio', 'Ex agente de la CIA'), ('Remate', 'Rompe la tele con el brazo de acero')]),
        'ching': dict(name='PROF. CHING', esc='Fábrica', tcor='1,1 s',
            bio='El misterioso científico oriental que va en una silla de ruedas asesina diseñada por él. Aunque tiene una '
                'discapacidad, es un famoso maestro de artes marciales chinas.',
            pros=['Corriendo es el más rápido con diferencia (8,0).', 'Con el lanzacohetes dispara perfectamente recto: el único.',
                  'Salta bastante (69).'],
            cons=['El que más despacio anda.', 'Recibe un 30 % más que Bernard.', 'Mala puntería con pistola y metralleta.'],
            cur=['Lo llamaron el «Einstein asiático» durante la Segunda Guerra Mundial y, después, en la Guerra Fría.',
                 'Las artes marciales chinas que domina para matar se asocian con la longevidad y la salud.'],
            wiki=[('Nombre', 'Prof. Hon Ching'), ('Edad', '82 años (1912)'), ('Altura', '1,57 m · 45 kg'), ('Origen', 'Pekín (China)'),
                  ('Oficio', 'Científico, "el Einstein asiático"'), ('Remate', 'Pirata la tele y teclea "DISPOSED"')]),
        'dweeb': dict(name='DWEEB', esc='Avión', tcor='0,4 s',
            bio='El único asesino profesional conocido que cobra en plátanos. Es el hijo secuestrado del chimpancé más listo del '
                'mundo, estrella de la tele: "Mr. Happy".',
            pros=['El más pequeño (2 losetas de alto): el más difícil de acertar.', 'Salta mucho (72).', 'Arranca a correr enseguida.'],
            cons=['El que peor apunta de todo el juego.', 'Recibe un 20 % más que Bernard.', 'Anda despacio.'],
            cur=['Su pequeño cuerpo le hace menos vulnerable a los ataques enemigos.',
                 'Dispara muy mal: la trayectoria de su arma es extremadamente desequilibrada.'],
            wiki=[('Edad', '10 años (1984)'), ('Altura', '84 cm'), ('Peso', '28 kg'), ('Origen', 'Un zoo; de origen africano'),
                  ('Oficio', 'Asesino a cambio de plátanos'), ('Remate', 'Pone en la tele un programa infantil')]),
        'gemelos': dict(name='DANNY Y DEMI', esc='Barco', tcor='0,3 s',
            bio='Los ex siameses, separados en un accidente de tren. Además de ser muy inteligentes, los dos tienen una mente '
                'siniestra y criminal.',
            pros=['Pueden llevar dos armas: guardan una de reserva y la sacan solos al gastar la de la mano.',
                  'Los que antes arrancan a correr (0,3 s).', 'Placaje fuerte (6).'],
            cons=['Los más frágiles: reciben un 33 % más que Bernard.', 'Los que menos saltan (53).', 'Mala puntería con cohetes.'],
            cur=['Nacieron en el laboratorio de un fisiólogo loco y se criaron en una jaula, como inocentes hámsteres de experimento.',
                 'Nada les gusta más que jugar a la guerra con extraños usando «juguetes» como pistolas y cuchillos.'],
            wiki=[('Apellido', 'Fritz'), ('Edad', '9 años (1985)'), ('Altura', 'Danny 1,30 · Demi 1,32 m'), ('Peso', '35 kg cada uno'),
                  ('Origen', 'Berlín (Alemania)'), ('Remate', 'Pintarrajean la tele con cera verde')]),
    },
)

# ======================================================================== ENGLISH
T['en'] = dict(
    cover_title='GAME MANUAL',
    cover_sub='Story  ·  Characters  ·  Weapons  ·  Stages  ·  Endings',
    sec_story='Main story', story_title='SEVEN KILLERS, ONE SURVIVOR', story_panel="Mr. Acme's contract",
    story=['A mystery man named Mr. Acme hired seven killers to assassinate seven world-famous art collectors. Oddly enough, each '
           'killer was also hired to eliminate the other six...',
           "What is Mr. Acme's motive? What is in store for the survivor? A large reward and the reputation of being the number one "
           'killer in the world?',
           'You pick one of the seven. Before each fight, Mr. Acme gives you your orders on TV: who your next target is, how much '
           "you'll be paid and where to find them. Every fight takes place on the target's turf, and the only rule is the one on "
           'the sign that opens it: take out your enemy by any means!',
           "Once you've eliminated the other six, one last job remains: making your way into Mr. Acme's own mansion."],
    howto_title='HOW TO PLAY', rules_panel='Fight rules',
    rules=["Everyone starts with a full energy bar. Nobody has more life than anyone else: what changes is how much each hit takes.",
           '200 seconds per fight. When time runs out, the player with less life loses.',
           'Weapons keep appearing around the stage: gun, machine gun, rocket launcher, grenade, katana, whip and flamethrower. '
           'You can also throw whatever is at hand: soup, cake, a fruit basket or coal.',
           "After being hit, a fighter blinks for a second and can't be hit again.",
           'Tap a direction twice to run; while running you jump further, slide by crouching and can tackle.',
           'The stage fights back too: falling into the void takes energy, and every place has its own traps.'],
    dmg_panel='Damage of each attack', dmg_cols=('ATTACK', 'BASE', "JOHN'S BAR"),
    dmg_rows=['Rocket / grenade blast', 'Falling into the void', 'Gun (per bullet)', 'Katana (per hit)', 'Flamethrower (per touch)',
              'Soup', 'Whip (per hit)', 'Machine gun (per bullet)', 'Punch (Bernard: 3)', 'Cake'],
    dmg_note="Base damage from the ROM. The percentage is what it takes from John Smith on Normal; for the others, it depends on "
             'how tough each one is (see the character pages).',
    sec_chars='Character profiles',
    p_data='Data', p_who='Who is it', stage_lbl='Stage: ', p_stats='Stats', p_arms='Weapons and hits', p_pros='Pros',
    p_cons='Cons', p_cur='Trivia',
    st=('Walk', 'Run', 'Jump', 'Toughness', 'Aim'), st_recv='takes %d %%', st_gun='gun %d°',
    arms=('Gun spread', 'With rocket launcher', 'With machine gun', 'Running tackle', 'Punch'),
    cmp_title='COMPARISON', cmp_panel='The seven, head to head',
    cmp_cols=['Character', 'Walk', 'Run', 'Jump', 'Takes', 'Gun', 'Rockets', 'Mach. gun', 'Tackle'],
    cmp_note='In green, the best of each column. Walk and run in pixels per frame; jump, height in pixels; takes, damage compared '
             'with Bernard (100 %); weapons, spread of each shot (less = straighter).',
    acme_panel='The one who started it all',
    acme=['The mysterious client who hired the seven killers and who, it seems, wanted only one of them left standing. He always '
          'speaks through the TV, giving the orders for each job.',
          'In the end he waits in his mansion with Mrs. Acme. Outside, he attacks from his helicopter; to shoot it down there is a '
          'special rocket launcher (SPECIAL ROCKET) with UNLIMITED ammo.',
          'His mansion is made for fighting the CPU: that is why it is not used in two-player fights.'],
    sec_weap='Weapons', weap_title='THE ARSENAL', ammo_lbl='Ammo: ', dmg_lbl='Damage: ',
    weap=[('GUN', '7 bullets', '20', "The most common one. Watch out for running out of ammo! Betty fires it the straightest."),
          ('MACHINE GUN', '80 bullets', '2 per bullet', 'Low damage, but it fires fast. After each hit the rival is immune for a second.'),
          ('ROCKET LAUNCHER', '3 rockets', '35', 'The hardest hitter. Mind the recoil! Ching fires it perfectly straight.'),
          ('FLAMETHROWER', '300 gas', '10 per touch', 'It burns everything to ashes! After each touch the rival is immune for a '
           'second: the damage does not stack.'),
          ('GRENADE', '1', '35 (blast)', "Pull the pin and throw it. If one is thrown at you, you can throw it back! Eve's lizard "
           'knows how too.'),
          ('KATANA', 'unlimited', '15', 'Hold the attack button to shield yourself from bullets, and you can even run like that. '
           'But careful: the machine gun can break your katana.'),
          ('WHIP', 'unlimited', '5', 'Strikes and knocks rivals down at just the right moment.')],
    use_panel='How to use them',
    use=['Crouch over a weapon to pick it up.', 'You can only carry one (the twins carry a spare).',
         'Once out of ammo, the weapon is eventually lost.',
         'Bullets can collide with each other, and you can even detonate a missile by hitting it with a bullet.'],
    obj_title='ITEMS AND TRAPS', throw_panel='Everything you can throw',
    throw='Walk up to boxes, drums and the like, and hold the ATTACK button to pick them up and carry them; let go to throw '
          'them. There may be unexpected items... use them wisely!',
    throw_rows=[('Soup', '10'), ('Grenade (if it hits without exploding)', '1'), ('Coal (train)', '1'), ('Basket fruit (6 pieces)', '0 to 2'),
                ('Cake', '0')],
    traps_panel='Stage weapons and traps',
    traps=['Plane: machine guns mounted on the plane itself.', 'Circus: clowns drop guns when shot, some of them toys.',
           'Factory: the elevators bring up rocket launchers.', 'Train: there are very hot items inside the middle car.',
           "Mansion: a special rocket launcher with UNLIMITED ammo for Mr. Acme's helicopter.",
           'Falling into the void or the water takes energy (25 base).'],
    sec_stages='Stages', turf='Turf of ',
    stages=[('SKYSCRAPER', 'John Smith', 'An office building from top to bottom. The fight starts on the roof, next to a '
             'helicopter; about halfway through, the helicopter explodes and everyone falls into the dining room on the floor '
             'below. In the basement, left of the garage, there is a pit with seven grenades in a row.'),
            ('CIRCUS', 'Eve', 'A circus big top, with trapezes and ropes above the ring and the audience all around. The trapezes '
             'take you from one side to the other. As time goes by, clowns come out and, if you shoot them, they drop guns (maybe '
             'fake ones), along with a tiger, an elephant and the human cannonball cannon.'),
            ('AQUARIUM', 'Betty Doe', 'An aquarium with a whale in its tank, fish and several floors linked by stairs. After a '
             "while a bomb goes off on the whale's mount, breaking the aquarium's columns and filling it with water and all the "
             'sea creatures that were locked up.'),
            ('TRAIN', 'Bernard White', 'The fight takes place on top of a moving steam train among the mountains: cars, a tender '
             'full of coal to throw and the locomotive. Whoever falls off the front or the back climbs back on a handcar. If you '
             'break the walls of the middle car, there are very hot items inside.'),
            ('FACTORY', 'Prof. Ching', "The professor's factory: a maze of catwalks, stairs, conveyor belts and elevators that "
             'bring up rockets. It is one of the biggest stages, and the camera zooms far out to show it all. A paradise of '
             'rockets and bullets.'),
            ('PLANE', 'Dweeb', 'A plane in mid-flight, outside and inside, with a helicopter around it. It has mounted machine '
             'guns, so make good use of them. If someone falls into the void, they come back with less energy.'),
            ('SHIP', 'Danny & Demi', 'A yacht on the high seas with several decks, from the bridge to the holds. The sea is on '
             'both sides: whoever falls into the water climbs back on board... if they are not eaten first. The biggest challenge '
             'of this level is aiming.'),
            ('MANSION', 'Mr. Acme', "The last job: Mr. Acme's mansion, inside and out. Inside there are dogs, sea creatures, traps "
             'and explosions, all to stop you from reaching the final showdown with Mr. Acme.')],
    sec_end='Endings', end_title='THE SURVIVOR', end_panel='After beating Mr. Acme',
    end='If you take out Mr. Acme and his boss (his wife), you will see your character\'s ending with the financial result of '
        'your work killing your rivals.',
    end_rows=[('REWARD', 'what Mr. Acme pays for the jobs'), ('HOSPITAL BILL', 'the bill for your wounds'),
              ('PROFIT', 'what you keep'), ('BANK BALANCE', 'and, finally, your account')],
    end_tag='The less you get hurt, the more money!', end_cap="Eve's accounts after taking out Mr. Acme",
    pending='PHOTO PENDING',
    pages=dict(cover='Cover', story='Story', howto='How to play', card='Profile %s', cmp='Comparison', acme='Mr. Acme',
               weap='Weapons', obj='Items', stage='Stage %s', end='Endings', fin='Ending %s'),
    pdf='The Outfoxies Remastered - Manual (English).pdf',
    ch={
        'john': dict(name='JOHN SMITH', esc='Skyscraper', tcor='0.4 s',
            bio='The man who would do anything, from babysitting to agitating a revolution, to get money.',
            pros=['The most balanced: neither weak nor slow at anything.', 'The second toughest.', 'Good aim with every weapon.'],
            cons=['He does not stand out at anything.', 'Average running speed (4.0).'],
            cur=['Son of an officer of the Japanese occupation forces, he learned the secrets of Zen.',
                 'He is blind in his left eye.'],
            wiki=[('Age', '38 (born 1956)'), ('Height', '1.80 m'), ('Weight', '80 kg'), ('Origin', 'USA (born in Hiroshima)'),
                  ('Job', 'Mercenary'), ('Finisher', 'Sticks a "DONE" label on the TV')]),
        'eve': dict(name='EVE', esc='Circus', tcor='0.5 s',
            bio='A has-been film starlet, now a thief to support her lavish lifestyle. She has the skill to break into Nox fortress '
                'with the aid of her well-trained lizard.',
            pros=['The fastest walker in the whole game.', 'Jumps higher than anyone: reaches platforms others cannot.',
                  'Quite tough for someone so agile.'],
            cons=['Aim slightly worse than John.', 'One of the weakest tackles (4).'],
            cur=['The lizard on her back acts as a barrier that blocks enemy bullets, but only once.',
                 'Once it has blocked them, it drops and moves on its own, lighting the falling grenades and carrying them off.'],
            wiki=[('Name', 'Lisa "Eve" Price'), ('Age', '29 (born 1965)'), ('Height', '1.65 m · 50 kg'), ('Origin', 'London (UK)'),
                  ('Job', 'Thief and killer; formerly an actress'), ('Finisher', 'Shoots the TV')]),
        'betty': dict(name='BETTY DOE', esc='Aquarium', tcor='instantly',
            bio='A successful international businesswoman who desperately needs capital to fund her several businesses. Her '
                'diverse background includes being a former high-paid hit woman.',
            pros=['Fires the straightest with every weapon (gun 4°).', 'Reaches her running speed instantly.'],
            cons=['The slowest runner.', 'Jumps low and takes 17 % more than Bernard.'],
            cur=['One of the "most successful businesswomen of the 20th century".',
                 'Born in a poor neighbourhood, she rose to the third-highest personal income in the USA.'],
            wiki=[('Age', '33 (born 1961)'), ('Height', '1.57 m'), ('Weight', '43 kg'), ('Origin', 'Los Angeles (USA)'),
                  ('Job', 'Self-made businesswoman'), ('Finisher', 'Switches off the TV')]),
        'bernard': dict(name='BERNARD WHITE', esc='Train', tcor='1.2 s',
            bio='A biotech engineer with superhuman strength. An experiment cost him his hand: he now has an artificial hand made '
                'of steel.',
            pros=['The toughest of them all.', 'His punch hits three times harder than anyone else\'s.', 'The strongest tackle (12).',
                  'The second fastest walker.'],
            cons=['Jumps low (55).', 'The slowest to start running.', 'His gun pulls to one side.'],
            cur=['He hates killing animals, but feels no remorse whatsoever about killing people.',
                 'He is currently breaking the world record for mass murders with his own bare hands.'],
            wiki=[('Age', '36 (born 1958)'), ('Height', '2.16 m'), ('Weight', '135 kg'), ('Origin', 'Washington D.C. (USA)'),
                  ('Job', 'Former CIA agent'), ('Finisher', 'Smashes the TV with his steel arm')]),
        'ching': dict(name='PROF. CHING', esc='Factory', tcor='1.1 s',
            bio='The mysterious oriental scientist who rides a killer wheelchair he designed himself. Though partially handicapped, '
                'he is a famous master of Chinese martial arts.',
            pros=['By far the fastest runner (8.0).', 'Fires the rocket launcher perfectly straight: the only one.',
                  'Jumps quite high (69).'],
            cons=['The slowest walker.', 'Takes 30 % more than Bernard.', 'Poor aim with the gun and machine gun.'],
            cur=['He was called the "Asian Einstein" during World War II and, later, during the Cold War.',
                 'The Chinese martial arts he masters for killing are associated with longevity and health.'],
            wiki=[('Name', 'Prof. Hon Ching'), ('Age', '82 (born 1912)'), ('Height', '1.57 m · 45 kg'), ('Origin', 'Beijing (China)'),
                  ('Job', 'Scientist, "the Asian Einstein"'), ('Finisher', 'Hacks the TV and types "DISPOSED"')]),
        'dweeb': dict(name='DWEEB', esc='Plane', tcor='0.4 s',
            bio='The only known professional killer who works for bananas. He is the kidnapped son of the famous TV star, the '
                'world\'s most intelligent chimpanzee: "Mr. Happy".',
            pros=['The smallest (2 tiles tall): the hardest to hit.', 'Jumps high (72).', 'Starts running right away.'],
            cons=['The worst aim in the whole game.', 'Takes 20 % more than Bernard.', 'Walks slowly.'],
            cur=['His small body makes him less vulnerable to enemy attacks.',
                 'He is a terrible shot: his weapon\'s trajectory is extremely unbalanced.'],
            wiki=[('Age', '10 (born 1984)'), ('Height', '84 cm'), ('Weight', '28 kg'), ('Origin', 'A zoo; African-born'),
                  ('Job', 'Killer for bananas'), ('Finisher', "Puts a kids' show on the TV")]),
        'gemelos': dict(name='DANNY & DEMI', esc='Ship', tcor='0.3 s',
            bio='The ex-Siamese twins, separated by a train crash. Besides being extremely intelligent, they both possess a '
                'sinister and criminal mind.',
            pros=['They can carry two weapons: they keep a spare and draw it on their own when the first one runs out.',
                  'The quickest to start running (0.3 s).', 'Strong tackle (6).'],
            cons=['The frailest: they take 33 % more than Bernard.', 'The lowest jumpers (53).', 'Poor aim with rockets.'],
            cur=['Born in a mad physiologist\'s lab, they were raised in a cage like innocent experimental hamsters.',
                 'Nothing pleases them more than playing war with strangers, using "toys" like guns and knives.'],
            wiki=[('Surname', 'Fritz'), ('Age', '9 (born 1985)'), ('Height', 'Danny 1.30 · Demi 1.32 m'), ('Weight', '35 kg each'),
                  ('Origin', 'Berlin (Germany)'), ('Finisher', 'Scribble on the TV with green crayon')]),
    },
)

# ======================================================================== PORTUGUES (de Portugal)
T['pt'] = dict(
    cover_title='MANUAL DO JOGO',
    cover_sub='História  ·  Personagens  ·  Armas  ·  Cenários  ·  Finais',
    sec_story='História principal', story_title='SETE ASSASSINOS, UM SÓ SOBREVIVENTE', story_panel='A encomenda de Mr. Acme',
    story=['Um homem misterioso chamado Mr. Acme contratou sete assassinos para matar sete famosos colecionadores de arte do mundo. '
           'Curiosamente, a cada assassino também encomendou eliminar os outros seis...',
           'Qual é o motivo de Mr. Acme? O que espera o sobrevivente? Uma grande recompensa e a fama de ser o melhor assassino do mundo?',
           'Escolhes um dos sete. Antes de cada combate, Mr. Acme dá-te as instruções pela televisão: quem é o teu próximo alvo, '
           'quanto te paga e onde o encontrar. Cada combate decorre no terreno do alvo, e a única regra é a do cartaz que o abre: '
           'acaba com o teu inimigo seja como for!',
           'Quando tiveres eliminado os outros seis, falta a última encomenda: chegar à mansão do próprio Mr. Acme.'],
    howto_title='COMO SE JOGA', rules_panel='Regras do combate',
    rules=['Todos começam com a barra de energia cheia. Ninguém tem mais vida do que outro: o que muda é quanto lhe tira cada golpe.',
           '200 segundos por combate. Se o tempo acabar, perde o jogador com menos vida.',
           'As armas vão aparecendo pelo cenário: pistola, metralhadora, lança-foguetes, granada, katana, chicote e lança-chamas. '
           'Também se pode atirar o que estiver à mão: sopa, bolo, cesto de fruta ou carvão.',
           'Depois de levar um golpe, o lutador pisca durante um segundo e não lhe podem voltar a acertar.',
           'Com dois toques seguidos para um lado corre-se; a correr salta-se mais longe, desliza-se agachando e faz-se uma placagem.',
           'O cenário também ataca: cair no vazio tira energia e há armadilhas próprias de cada sítio.'],
    dmg_panel='Dano de cada ataque', dmg_cols=('ATAQUE', 'BASE', 'BARRA DO JOHN'),
    dmg_rows=['Foguete / explosão de granada', 'Cair no vazio', 'Pistola (por bala)', 'Katana (por golpe)', 'Lança-chamas (por toque)',
              'Sopa', 'Chicote (por golpe)', 'Metralhadora (por bala)', 'Murro (Bernard: 3)', 'Bolo'],
    dmg_note='Dano base da ROM. A percentagem é o que tira ao John Smith em Normal; aos outros, conforme o que aguenta cada um '
             '(ver as fichas).',
    sec_chars='Fichas das personagens',
    p_data='Dados', p_who='Quem é', stage_lbl='Cenário: ', p_stats='Estatísticas', p_arms='Armas e golpes', p_pros='Prós',
    p_cons='Contras', p_cur='Curiosidades',
    st=('Andar', 'Correr', 'Salto', 'Resistência', 'Pontaria'), st_recv='recebe %d %%', st_gun='pistola %d°',
    arms=('Leque com pistola', 'Com lança-foguetes', 'Com metralhadora', 'Placagem a correr', 'Murro'),
    cmp_title='COMPARAÇÃO', cmp_panel='Os sete, frente a frente',
    cmp_cols=['Personagem', 'Andar', 'Correr', 'Salto', 'Recebe', 'Pistola', 'Foguetes', 'Metralh.', 'Placagem'],
    cmp_note='A verde, o melhor de cada coluna. Andar e correr em píxeis por fotograma; salto, altura em píxeis; recebe, dano '
             'comparado com o Bernard (100 %); armas, leque de desvio de cada disparo (menos = mais reto).',
    acme_panel='O que começou tudo',
    acme=['O misterioso cliente que contratou os sete assassinos e que, pelos vistos, queria que só restasse um. Fala sempre '
          'através da televisão, com as instruções de cada encomenda.',
          'No fim espera na sua mansão ao lado de Mrs. Acme. Lá fora, ataca do seu helicóptero; para o abater há um lança-mísseis '
          'especial (SPECIAL ROCKET) com munições ILIMITADAS.',
          'A sua mansão foi feita para lutar contra a máquina: por isso não aparece no combate entre dois jogadores.'],
    sec_weap='Armas', weap_title='O ARSENAL', ammo_lbl='Munições: ', dmg_lbl='Dano: ',
    weap=[('PISTOLA', '7 balas', '20', 'A mais comum. Cuidado para não ficares sem munições! A Betty é quem a dispara mais reta.'),
          ('METRALHADORA', '80 balas', '2 por bala', 'Pouco dano, mas dispara depressa. Depois de cada impacto, o rival fica imune '
           'durante um segundo.'),
          ('LANÇA-FOGUETES', '3 foguetes', '35', 'O que tira mais. Cuidado com o recuo! O Ching dispara-o perfeitamente reto.'),
          ('LANÇA-CHAMAS', '300 de gás', '10 por toque', 'Reduz tudo a cinzas! Depois de cada toque, o rival fica imune durante um '
           'segundo: o dano não se acumula.'),
          ('GRANADA', '1', '35 (explosão)', 'Tira a cavilha e atira-a. Se ta atirarem, podes devolvê-la! O lagarto da Eve também sabe.'),
          ('KATANA', 'sem limite', '15', 'Se mantiveres o botão de ataque premido, usá-la-ás para te protegeres das balas, podendo '
           'até correr assim. Mas cuidado: a metralhadora pode partir-te a katana.'),
          ('CHICOTE', 'sem limite', '5', 'Ataca e derruba os rivais no momento certo.')],
    use_panel='Como se usam',
    use=['Agacha-te em cima de uma arma para a apanhar.', 'Só se leva uma (os gémeos, uma de reserva).',
         'Sem munições, a arma acaba por se perder.',
         'As balas podem chocar entre si, e podes até detonar um míssil se lhe acertares com uma bala.'],
    obj_title='OBJETOS E ARMADILHAS', throw_panel='Tudo o que se pode atirar',
    throw='Aproxima-te de caixas, bidões e afins, e mantém premido o botão de ATAQUE para os apanhar e levar; larga-os para os '
          'atirar. Pode haver objetos inesperados... usa-os com cabeça!',
    throw_rows=[('Sopa', '10'), ('Granada (se acertar sem explodir)', '1'), ('Carvão (comboio)', '1'), ('Fruta do cesto (6 peças)', '0 a 2'),
                ('Bolo', '0')],
    traps_panel='Armas e armadilhas dos cenários',
    traps=['Avião: metralhadoras montadas no próprio avião.', 'Circo: os palhaços largam pistolas quando se dispara contra eles, '
           'algumas de brincar.', 'Fábrica: os elevadores trazem lança-foguetes.', 'Comboio: dentro da carruagem do meio há objetos '
           'muito quentes.', 'Mansão: lança-mísseis especial com munições ILIMITADAS para o helicóptero de Mr. Acme.',
           'Cair no vazio ou à água tira energia (25 de base).'],
    sec_stages='Cenários', turf='Terreno de ',
    stages=[('ARRANHA-CÉUS', 'John Smith', 'Um edifício de escritórios de cima a baixo. O combate começa no terraço, junto a um '
             'helicóptero; a meio do tempo, o helicóptero explode e todos caem para a cantina do andar de baixo. Na cave, à '
             'esquerda da garagem, há um fosso com sete granadas em fila.'),
            ('CIRCO', 'Eve', 'A tenda de um circo, com trapézios e cordas por cima da pista e o público à volta. Os trapézios servem '
             'para atravessar de um lado ao outro. À medida que o tempo passa saem palhaços que, se lhes disparares, largam '
             'pistolas (talvez falsas), além de um tigre, um elefante e o canhão do homem-bala.'),
            ('AQUÁRIO', 'Betty Doe', 'Um aquário com uma baleia no seu tanque, peixes e vários pisos ligados por escadas. Com o '
             'tempo explode uma bomba no suporte da baleia, que parte as colunas do aquário e o vai enchendo de água e de todas as '
             'criaturas marinhas que estavam presas.'),
            ('COMBOIO', 'Bernard White', 'Luta-se em cima de um comboio a vapor em andamento, entre montanhas: carruagens, tênder '
             'com carvão para atirar e a locomotiva. Quem cai do comboio pela frente ou por trás volta a subir numa vagoneta. Se '
             'partires as paredes da carruagem do meio, há objetos muito quentes.'),
            ('FÁBRICA', 'Prof. Ching', 'A fábrica do professor: um labirinto de passadiços, escadas, tapetes rolantes e elevadores '
             'que trazem foguetes. É dos cenários maiores, e a câmara afasta-se muito para o mostrar inteiro. O paraíso dos foguetes '
             'e das balas.'),
            ('AVIÃO', 'Dweeb', 'Um avião em pleno voo, por fora e por dentro, com um helicóptero à volta. Tem metralhadoras montadas, '
             'por isso aproveita-as. Se alguém cair no vazio, volta com menos energia.'),
            ('BARCO', 'Danny e Demi', 'Um iate em alto mar com vários conveses, da ponte aos porões. Dos lados está o mar: quem cai à '
             'água acaba por voltar a bordo... se não for comido antes. O maior desafio deste nível é conseguir apontar.'),
            ('MANSÃO', 'Mr. Acme', 'A última encomenda: a mansão de Mr. Acme, por dentro e por fora. Lá dentro há cães, animais '
             'aquáticos, armadilhas e explosões para que não chegues ao confronto final com Mr. Acme.')],
    sec_end='Finais', end_title='O SOBREVIVENTE', end_panel='Ao vencer Mr. Acme',
    end='Se acabares com Mr. Acme e com a sua chefe (a esposa), verás o final da tua personagem com o resultado económico do teu '
        'trabalho a matar os teus rivais.',
    end_rows=[('RECOMPENSA', 'o que Mr. Acme paga pelas encomendas'), ('HOSPITAL', 'a fatura dos teus ferimentos'),
              ('LUCRO', 'o que te fica limpo'), ('SALDO EM CONTA', 'e, por fim, a tua conta')],
    end_tag='Quanto menos te ferirem, mais dinheiro!', end_cap='As contas da Eve depois de acabar com Mr. Acme',
    pending='FOTO PENDENTE',
    pages=dict(cover='Capa', story='Historia', howto='Como se joga', card='Ficha %s', cmp='Comparacao', acme='Mr. Acme',
               weap='Armas', obj='Objetos', stage='Cenario %s', end='Finais', fin='Final %s'),
    pdf='The Outfoxies Remastered - Manual (Portugues).pdf',
    ch={
        'john': dict(name='JOHN SMITH', esc='Arranha-céus', tcor='0,4 s',
            bio='O homem capaz de fazer tudo, de cuidar de crianças a incitar uma revolução, para ganhar dinheiro.',
            pros=['O mais equilibrado: nem fraco nem lento em nada.', 'O segundo que menos dano recebe.', 'Boa pontaria com todas as armas.'],
            cons=['Não se destaca em nada concreto.', 'A correr é dos normais (4,0).'],
            cur=['Filho de um oficial das forças de ocupação japonesas, aprendeu os segredos do zen.',
                 'É cego do olho esquerdo.'],
            wiki=[('Idade', '38 anos (1956)'), ('Altura', '1,80 m'), ('Peso', '80 kg'), ('Origem', 'EUA (nasceu em Hiroxima)'),
                  ('Ofício', 'Mercenário'), ('Remate', 'Cola na TV uma etiqueta de "FEITO"')]),
        'eve': dict(name='EVE', esc='Circo', tcor='0,5 s',
            bio='Antiga estrela de cinema, agora ladra para pagar a sua vida de luxo. Consegue entrar na fortaleza de Nox com a '
                'ajuda do seu lagarto bem treinado.',
            pros=['A mais rápida a andar em todo o jogo.', 'Salta mais alto do que ninguém: chega a plataformas que os outros não alcançam.',
                  'Aguenta bem para ser tão ágil.'],
            cons=['Pontaria um pouco pior do que a do John.', 'A sua placagem é das fracas (4).'],
            cur=['O lagarto que leva às costas faz de barreira e bloqueia as balas inimigas, mas só uma vez.',
                 'Depois de as bloquear, cai e move-se sozinho, acendendo as granadas que caem e levando-as consigo.'],
            wiki=[('Nome', 'Lisa "Eve" Price'), ('Idade', '29 anos (1965)'), ('Altura', '1,65 m · 50 kg'), ('Origem', 'Londres (Reino Unido)'),
                  ('Ofício', 'Ladra e assassina; antes, atriz'), ('Remate', 'Dispara contra a TV')]),
        'betty': dict(name='BETTY DOE', esc='Aquário', tcor='de imediato',
            bio='Uma empresária internacional de sucesso que precisa urgentemente de capital para financiar os seus muitos negócios. '
                'O seu passado variado inclui ter sido uma assassina muito bem paga.',
            pros=['A que dispara mais reto com qualquer arma (pistola 4°).', 'Chega à velocidade de corrida de imediato.'],
            cons=['A correr é a mais lenta.', 'Salta pouco e recebe 17 % mais do que o Bernard.'],
            cur=['Uma das «empresárias mais bem-sucedidas do século XX».',
                 'Passou de nascer num bairro pobre a ter o terceiro maior rendimento pessoal dos EUA.'],
            wiki=[('Idade', '33 anos (1961)'), ('Altura', '1,57 m'), ('Peso', '43 kg'), ('Origem', 'Los Angeles (EUA)'),
                  ('Ofício', 'Empresária feita por si própria'), ('Remate', 'Desliga a TV')]),
        'bernard': dict(name='BERNARD WHITE', esc='Comboio', tcor='1,2 s',
            bio='Engenheiro biotecnológico com força sobre-humana. Uma experiência fê-lo perder uma mão: agora tem uma mão artificial '
                'de aço.',
            pros=['O que menos dano recebe de todos.', 'O seu murro bate o triplo do dos outros.', 'A placagem mais forte (12).',
                  'O segundo mais rápido a andar.'],
            cons=['Salta pouco (55).', 'É o que mais demora a começar a correr.', 'A sua pistola desvia-se para um lado.'],
            cur=['Odeia matar animais, mas não sente qualquer remorso por matar pessoas.',
                 'Está a bater o recorde mundial de assassínios em massa com as próprias mãos.'],
            wiki=[('Idade', '36 anos (1958)'), ('Altura', '2,16 m'), ('Peso', '135 kg'), ('Origem', 'Washington D. C. (EUA)'),
                  ('Ofício', 'Ex-agente da CIA'), ('Remate', 'Parte a TV com o braço de aço')]),
        'ching': dict(name='PROF. CHING', esc='Fábrica', tcor='1,1 s',
            bio='O misterioso cientista oriental que anda numa cadeira de rodas mortal criada por ele. Embora tenha uma deficiência, '
                'é um famoso mestre de artes marciais chinesas.',
            pros=['A correr é de longe o mais rápido (8,0).', 'Com o lança-foguetes dispara perfeitamente reto: o único.',
                  'Salta bastante (69).'],
            cons=['O que anda mais devagar.', 'Recebe 30 % mais do que o Bernard.', 'Má pontaria com pistola e metralhadora.'],
            cur=['Chamaram-lhe o «Einstein asiático» durante a Segunda Guerra Mundial e, depois, na Guerra Fria.',
                 'As artes marciais chinesas que domina para matar estão associadas à longevidade e à saúde.'],
            wiki=[('Nome', 'Prof. Hon Ching'), ('Idade', '82 anos (1912)'), ('Altura', '1,57 m · 45 kg'), ('Origem', 'Pequim (China)'),
                  ('Ofício', 'Cientista, "o Einstein asiático"'), ('Remate', 'Pirateia a TV e escreve "DISPOSED"')]),
        'dweeb': dict(name='DWEEB', esc='Avião', tcor='0,4 s',
            bio='O único assassino profissional conhecido que cobra em bananas. É o filho raptado do chimpanzé mais esperto do mundo, '
                'estrela da televisão: "Mr. Happy".',
            pros=['O mais pequeno (2 blocos de altura): o mais difícil de acertar.', 'Salta muito (72).', 'Começa a correr logo.'],
            cons=['O que pior aponta em todo o jogo.', 'Recebe 20 % mais do que o Bernard.', 'Anda devagar.'],
            cur=['O seu corpo pequeno torna-o menos vulnerável aos ataques inimigos.',
                 'Dispara muito mal: a trajetória da sua arma é extremamente desequilibrada.'],
            wiki=[('Idade', '10 anos (1984)'), ('Altura', '84 cm'), ('Peso', '28 kg'), ('Origem', 'Um jardim zoológico; de origem africana'),
                  ('Ofício', 'Assassino a troco de bananas'), ('Remate', 'Põe na TV um programa infantil')]),
        'gemelos': dict(name='DANNY E DEMI', esc='Barco', tcor='0,3 s',
            bio='Os ex-siameses, separados num acidente de comboio. Além de muito inteligentes, os dois têm uma mente sinistra e '
                'criminosa.',
            pros=['Podem levar duas armas: guardam uma de reserva e sacam-na sozinhos quando gastam a da mão.',
                  'Os que começam a correr mais cedo (0,3 s).', 'Placagem forte (6).'],
            cons=['Os mais frágeis: recebem 33 % mais do que o Bernard.', 'Os que menos saltam (53).', 'Má pontaria com foguetes.'],
            cur=['Nasceram no laboratório de um fisiologista louco e cresceram numa jaula, como inocentes hamsters de laboratório.',
                 'Nada lhes agrada mais do que brincar à guerra com estranhos usando «brinquedos» como pistolas e facas.'],
            wiki=[('Apelido', 'Fritz'), ('Idade', '9 anos (1985)'), ('Altura', 'Danny 1,30 · Demi 1,32 m'), ('Peso', '35 kg cada um'),
                  ('Origem', 'Berlim (Alemanha)'), ('Remate', 'Rabiscam a TV com cera verde')]),
    },
)

# ======================================================================== ITALIANO
T['it'] = dict(
    cover_title='MANUALE DEL GIOCO',
    cover_sub='Storia  ·  Personaggi  ·  Armi  ·  Scenari  ·  Finali',
    sec_story='Storia principale', story_title='SETTE KILLER, UN SOLO SOPRAVVISSUTO', story_panel="L'incarico di Mr. Acme",
    story=["Un uomo misterioso chiamato Mr. Acme assoldò sette killer per uccidere sette famosi collezionisti d'arte del mondo. "
           'Stranamente, a ogni killer chiese anche di eliminare gli altri sei...',
           'Qual è il movente di Mr. Acme? Cosa aspetta il sopravvissuto? Una grande ricompensa e la fama di miglior killer del mondo?',
           'Scegli uno dei sette. Prima di ogni scontro, Mr. Acme ti dà le istruzioni in TV: chi è il tuo prossimo bersaglio, quanto '
           'ti paga e dove trovarlo. Ogni scontro si svolge sul terreno del bersaglio, e l\'unica regola è quella del cartello che '
           'lo apre: fai fuori il tuo nemico con ogni mezzo!',
           'Quando avrai eliminato gli altri sei, resta l\'ultimo incarico: arrivare fino alla villa dello stesso Mr. Acme.'],
    howto_title='COME SI GIOCA', rules_panel='Regole dello scontro',
    rules=["Tutti partono con la barra dell'energia piena. Nessuno ha più vita degli altri: cambia solo quanto toglie ogni colpo.",
           '200 secondi per scontro. Se il tempo finisce, perde il giocatore con meno vita.',
           'Le armi compaiono qua e là nello scenario: pistola, mitragliatrice, lanciarazzi, granata, katana, frusta e lanciafiamme. '
           'Si può anche lanciare quello che si ha sottomano: zuppa, torta, cesto di frutta o carbone.',
           'Dopo aver subito un colpo, il lottatore lampeggia per un secondo e non può essere colpito di nuovo.',
           'Con due tocchi di fila verso un lato si corre; correndo si salta più lontano, si scivola abbassandosi e si fa un placcaggio.',
           'Anche lo scenario attacca: cadere nel vuoto toglie energia e ogni posto ha le sue trappole.'],
    dmg_panel='Danno di ogni attacco', dmg_cols=('ATTACCO', 'BASE', 'BARRA DI JOHN'),
    dmg_rows=['Razzo / esplosione di granata', 'Cadere nel vuoto', 'Pistola (per proiettile)', 'Katana (per colpo)',
              'Lanciafiamme (per tocco)', 'Zuppa', 'Frusta (per colpo)', 'Mitragliatrice (per proiettile)', 'Pugno (Bernard: 3)', 'Torta'],
    dmg_note='Danno base della ROM. La percentuale è quanto toglie a John Smith a Normale; agli altri, secondo quanto resiste '
             'ciascuno (vedi le schede).',
    sec_chars='Schede dei personaggi',
    p_data='Dati', p_who='Chi è', stage_lbl='Scenario: ', p_stats='Statistiche', p_arms='Armi e colpi', p_pros='Pregi',
    p_cons='Difetti', p_cur='Curiosità',
    st=('Camminata', 'Corsa', 'Salto', 'Resistenza', 'Mira'), st_recv='subisce %d %%', st_gun='pistola %d°',
    arms=('Ventaglio con pistola', 'Con lanciarazzi', 'Con mitragliatrice', 'Placcaggio in corsa', 'Pugno'),
    cmp_title='CONFRONTO', cmp_panel='I sette, faccia a faccia',
    cmp_cols=['Personaggio', 'Camm.', 'Corsa', 'Salto', 'Subisce', 'Pistola', 'Razzi', 'Mitra', 'Placc.'],
    cmp_note='In verde, il migliore di ogni colonna. Camminata e corsa in pixel per fotogramma; salto, altezza in pixel; subisce, '
             'danno rispetto a Bernard (100 %); armi, ventaglio di deviazione di ogni colpo (meno = più dritto).',
    acme_panel='Quello che ha cominciato tutto',
    acme=['Il misterioso cliente che ha assoldato i sette killer e che, a quanto pare, voleva che ne restasse uno solo. Parla '
          'sempre attraverso la TV, con le istruzioni di ogni incarico.',
          "Alla fine aspetta nella sua villa insieme a Mrs. Acme. Fuori, attacca dal suo elicottero; per abbatterlo c'è un "
          'lanciamissili speciale (SPECIAL ROCKET) con munizioni ILLIMITATE.',
          'La sua villa è fatta per combattere contro la CPU: per questo non compare negli scontri tra due giocatori.'],
    sec_weap='Armi', weap_title="L'ARSENALE", ammo_lbl='Munizioni: ', dmg_lbl='Danno: ',
    weap=[('PISTOLA', '7 colpi', '20', 'La più comune. Attento a non restare senza munizioni! Betty è quella che spara più dritto.'),
          ('MITRAGLIATRICE', '80 colpi', '2 a colpo', 'Poco danno, ma spara veloce. Dopo ogni colpo il rivale è immune per un secondo.'),
          ('LANCIARAZZI', '3 razzi', '35', 'Quello che toglie di più. Attento al rinculo! Ching lo spara perfettamente dritto.'),
          ('LANCIAFIAMME', '300 di gas', '10 a tocco', 'Riduce tutto in cenere! Dopo ogni tocco il rivale è immune per un secondo: '
           'il danno non si somma.'),
          ('GRANATA', '1', '35 (esplosione)', 'Togli la sicura e lanciala. Se te la lanciano, puoi rimandarla indietro! Anche la '
           'lucertola di Eve sa farlo.'),
          ('KATANA', 'illimitata', '15', 'Tenendo premuto il tasto di attacco la userai per ripararti dai proiettili, e puoi perfino '
           'correre così. Ma attento: la mitragliatrice può romperti la katana.'),
          ('FRUSTA', 'illimitata', '5', 'Colpisce e atterra i rivali al momento giusto.')],
    use_panel='Come si usano',
    use=["Abbassati sopra un'arma per raccoglierla.", 'Se ne porta una sola (i gemelli, una di riserva).',
         "Senza munizioni, l'arma finisce per andare persa.",
         'I proiettili possono scontrarsi tra loro, e puoi perfino far esplodere un missile colpendolo con un proiettile.'],
    obj_title='OGGETTI E TRAPPOLE', throw_panel='Tutto quello che si può lanciare',
    throw='Avvicinati a casse, bidoni e simili e tieni premuto il tasto ATTACCO per prenderli e portarli; lasciali per lanciarli. '
          'Potrebbero esserci oggetti inaspettati... usali con la testa!',
    throw_rows=[('Zuppa', '10'), ('Granata (se colpisce senza esplodere)', '1'), ('Carbone (treno)', '1'), ('Frutta del cesto (6 pezzi)', '0 a 2'),
                ('Torta', '0')],
    traps_panel='Armi e trappole degli scenari',
    traps=["Aereo: mitragliatrici montate sull'aereo stesso.", 'Circo: i clown lasciano pistole se gli spari, alcune giocattolo.',
           'Fabbrica: gli ascensori portano su lanciarazzi.', 'Treno: dentro la carrozza centrale ci sono oggetti bollenti.',
           "Villa: lanciamissili speciale con munizioni ILLIMITATE per l'elicottero di Mr. Acme.",
           "Cadere nel vuoto o in acqua toglie energia (25 di base)."],
    sec_stages='Scenari', turf='Terreno di ',
    stages=[('GRATTACIELO', 'John Smith', "Un palazzo di uffici da cima a fondo. Lo scontro inizia sul tetto, accanto a un "
             "elicottero; a metà del tempo l'elicottero esplode e tutti cadono nella mensa del piano di sotto. Nel seminterrato, "
             "a sinistra del garage, c'è una fossa con sette granate in fila."),
            ('CIRCO', 'Eve', 'Il tendone di un circo, con trapezi e corde sopra la pista e il pubblico tutto intorno. I trapezi '
             'servono per attraversare da un lato all\'altro. Col passare del tempo escono dei clown che, se gli spari, lasciano '
             "pistole (forse finte), oltre a una tigre, un elefante e il cannone dell'uomo proiettile."),
            ('ACQUARIO', 'Betty Doe', 'Un acquario con una balena nella sua vasca, pesci e vari piani collegati da scale. Col tempo '
             "esplode una bomba sul sostegno della balena, che rompe le colonne dell'acquario e lo riempie d'acqua e di tutte le "
             'creature marine che erano rinchiuse.'),
            ('TRENO', 'Bernard White', 'Si combatte sopra un treno a vapore in corsa, tra le montagne: vagoni, tender pieno di '
             'carbone da lanciare e la locomotiva. Chi cade dal treno davanti o dietro risale su un carrello. Se rompi le pareti '
             'della carrozza centrale, ci sono oggetti bollenti.'),
            ('FABBRICA', 'Prof. Ching', 'La fabbrica del professore: un labirinto di passerelle, scale, nastri trasportatori e '
             'ascensori che portano su razzi. È uno degli scenari più grandi, e la telecamera si allontana molto per mostrarlo '
             'tutto. Il paradiso dei razzi e dei proiettili.'),
            ('AEREO', 'Dweeb', 'Un aereo in pieno volo, fuori e dentro, con un elicottero intorno. Ha mitragliatrici montate, quindi '
             'approfittane. Se qualcuno cade nel vuoto, torna con meno energia.'),
            ('NAVE', 'Danny e Demi', "Uno yacht in alto mare con vari ponti, dalla plancia alle stive. Ai lati c'è il mare: chi "
             "cade in acqua torna a bordo... sempre che non venga mangiato prima. La sfida più grande di questo livello è riuscire a mirare."),
            ('VILLA', 'Mr. Acme', "L'ultimo incarico: la villa di Mr. Acme, dentro e fuori. Dentro ci sono cani, animali acquatici, "
             'trappole ed esplosioni per impedirti di arrivare allo scontro finale con Mr. Acme.')],
    sec_end='Finali', end_title='IL SOPRAVVISSUTO', end_panel='Dopo aver battuto Mr. Acme',
    end='Se fai fuori Mr. Acme e il suo capo (sua moglie), vedrai il finale del tuo personaggio con il risultato economico del '
        'tuo lavoro uccidendo i rivali.',
    end_rows=[('RICOMPENSA', 'quanto paga Mr. Acme per gli incarichi'), ('OSPEDALE', 'il conto delle tue ferite'),
              ('PROFITTO', 'quello che ti resta pulito'), ('SALDO BANCARIO', 'e, per finire, il tuo conto')],
    end_tag='Meno ti feriscono, più soldi!', end_cap='I conti di Eve dopo aver fatto fuori Mr. Acme',
    pending='FOTO IN ARRIVO',
    pages=dict(cover='Copertina', story='Storia', howto='Come si gioca', card='Scheda %s', cmp='Confronto', acme='Mr. Acme',
               weap='Armi', obj='Oggetti', stage='Scenario %s', end='Finali', fin='Finale %s'),
    pdf='The Outfoxies Remastered - Manuale (Italiano).pdf',
    ch={
        'john': dict(name='JOHN SMITH', esc='Grattacielo', tcor='0,4 s',
            bio="L'uomo pronto a fare di tutto, dal babysitter allo scatenare una rivoluzione, pur di fare soldi.",
            pros=['Il più equilibrato: né debole né lento in niente.', 'Il secondo che subisce meno danno.', 'Buona mira con tutte le armi.'],
            cons=['Non eccelle in niente di preciso.', 'In corsa è nella media (4,0).'],
            cur=["Figlio di un ufficiale delle forze d'occupazione giapponesi, ha imparato i segreti dello zen.",
                 "È cieco dall'occhio sinistro."],
            wiki=[('Età', '38 anni (1956)'), ('Altezza', '1,80 m'), ('Peso', '80 kg'), ('Origine', 'USA (nato a Hiroshima)'),
                  ('Mestiere', 'Mercenario'), ('Colpo finale', 'Attacca sulla TV un\'etichetta "FATTO"')]),
        'eve': dict(name='EVE', esc='Circo', tcor='0,5 s',
            bio='Ex stellina del cinema, ora ladra per pagarsi una vita di lusso. Riesce a entrare nella fortezza di Nox con '
                "l'aiuto della sua lucertola ben addestrata.",
            pros=['La più veloce a camminare di tutto il gioco.', 'Salta più in alto di tutti: raggiunge piattaforme che gli altri non arrivano.',
                  'Resiste bene per essere così agile.'],
            cons=['Mira un po\' peggiore di John.', 'Il suo placcaggio è tra i più deboli (4).'],
            cur=['La lucertola che porta sulla schiena fa da barriera e blocca i proiettili nemici, ma solo una volta.',
                 'Dopo averli bloccati cade e si muove da sola, accendendo le granate che cadono e portandole via.'],
            wiki=[('Nome', 'Lisa "Eve" Price'), ('Età', '29 anni (1965)'), ('Altezza', '1,65 m · 50 kg'), ('Origine', 'Londra (Regno Unito)'),
                  ('Mestiere', 'Ladra e killer; prima, attrice'), ('Colpo finale', 'Spara alla TV')]),
        'betty': dict(name='BETTY DOE', esc='Acquario', tcor='subito',
            bio='Una famosa imprenditrice internazionale che ha urgente bisogno di capitali per finanziare i suoi numerosi affari. '
                'Il suo passato variegato comprende anche l\'essere stata una sicaria pagatissima.',
            pros=['Quella che spara più dritto con qualsiasi arma (pistola 4°).', 'Raggiunge subito la velocità di corsa.'],
            cons=['In corsa è la più lenta.', 'Salta poco e subisce il 17 % in più di Bernard.'],
            cur=['Una delle «imprenditrici di maggior successo del XX secolo».',
                 'Nata in un quartiere povero, è arrivata ad avere il terzo reddito personale più alto degli USA.'],
            wiki=[('Età', '33 anni (1961)'), ('Altezza', '1,57 m'), ('Peso', '43 kg'), ('Origine', 'Los Angeles (USA)'),
                  ('Mestiere', 'Imprenditrice che si è fatta da sé'), ('Colpo finale', 'Spegne la TV')]),
        'bernard': dict(name='BERNARD WHITE', esc='Treno', tcor='1,2 s',
            bio='Bioingegnere dotato di forza sovrumana. Un esperimento gli ha fatto perdere una mano: ora ha una mano artificiale '
                "d'acciaio.",
            pros=['Quello che subisce meno danno di tutti.', 'Il suo pugno colpisce il triplo di quello degli altri.',
                  'Il placcaggio più forte (12).', 'Il secondo più veloce a camminare.'],
            cons=['Salta poco (55).', 'È quello che ci mette di più a partire in corsa.', 'La sua pistola devia da un lato.'],
            cur=['Odia uccidere animali, ma non prova alcun rimorso a uccidere persone.',
                 'Sta battendo il record mondiale di omicidi di massa a mani nude.'],
            wiki=[('Età', '36 anni (1958)'), ('Altezza', '2,16 m'), ('Peso', '135 kg'), ('Origine', 'Washington D.C. (USA)'),
                  ('Mestiere', 'Ex agente della CIA'), ('Colpo finale', "Sfonda la TV col braccio d'acciaio")]),
        'ching': dict(name='PROF. CHING', esc='Fabbrica', tcor='1,1 s',
            bio='Il misterioso scienziato orientale che va su una sedia a rotelle killer progettata da lui. Pur essendo disabile, '
                'è un famoso maestro di arti marziali cinesi.',
            pros=['In corsa è di gran lunga il più veloce (8,0).', 'Con il lanciarazzi spara perfettamente dritto: l\'unico.',
                  'Salta parecchio (69).'],
            cons=['Il più lento a camminare.', 'Subisce il 30 % in più di Bernard.', 'Mira scarsa con pistola e mitragliatrice.'],
            cur=['Fu chiamato l\'«Einstein asiatico» durante la Seconda guerra mondiale e, poi, nella Guerra fredda.',
                 'Le arti marziali cinesi che padroneggia per uccidere sono legate alla longevità e alla salute.'],
            wiki=[('Nome', 'Prof. Hon Ching'), ('Età', '82 anni (1912)'), ('Altezza', '1,57 m · 45 kg'), ('Origine', 'Pechino (Cina)'),
                  ('Mestiere', 'Scienziato, "l\'Einstein asiatico"'), ('Colpo finale', 'Pirata la TV e scrive "DISPOSED"')]),
        'dweeb': dict(name='DWEEB', esc='Aereo', tcor='0,4 s',
            bio="L'unico killer professionista noto pagato in banane. È il figlio rapito del famoso divo della TV, lo scimpanzé "
                'più geniale del mondo: "Mr. Happy".',
            pros=['Il più piccolo (2 blocchi di altezza): il più difficile da colpire.', 'Salta molto (72).', 'Parte in corsa subito.'],
            cons=['Quello che mira peggio in tutto il gioco.', 'Subisce il 20 % in più di Bernard.', 'Cammina piano.'],
            cur=['Il suo corpo piccolo lo rende meno vulnerabile agli attacchi nemici.',
                 'Spara malissimo: la traiettoria della sua arma è estremamente squilibrata.'],
            wiki=[('Età', '10 anni (1984)'), ('Altezza', '84 cm'), ('Peso', '28 kg'), ('Origine', 'Uno zoo; di origine africana'),
                  ('Mestiere', 'Killer in cambio di banane'), ('Colpo finale', 'Mette in TV un programma per bambini')]),
        'gemelos': dict(name='DANNY E DEMI', esc='Nave', tcor='0,3 s',
            bio='Gli ex siamesi, separati da un incidente ferroviario. Oltre a essere intelligentissimi, hanno entrambi una mente '
                'sinistra e criminale.',
            pros=['Possono portare due armi: ne tengono una di riserva e la tirano fuori da soli quando finisce quella in mano.',
                  'I più rapidi a partire in corsa (0,3 s).', 'Placcaggio forte (6).'],
            cons=['I più fragili: subiscono il 33 % in più di Bernard.', 'Quelli che saltano meno (53).', 'Mira scarsa con i razzi.'],
            cur=['Nati nel laboratorio di un fisiologo pazzo, sono cresciuti in gabbia come innocenti criceti da esperimento.',
                 'Niente li diverte di più che giocare alla guerra con gli sconosciuti usando «giocattoli» come pistole e coltelli.'],
            wiki=[('Cognome', 'Fritz'), ('Età', '9 anni (1985)'), ('Altezza', 'Danny 1,30 · Demi 1,32 m'), ('Peso', '35 kg ciascuno'),
                  ('Origine', 'Berlino (Germania)'), ('Colpo finale', 'Scarabocchiano la TV con un pastello verde')]),
    },
)
