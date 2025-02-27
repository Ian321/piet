//// includes

// header file
#include "p_virtual_machine.h"

// piet core
#include "../debug.h"
#include "p_enums.h"
#include "p_structs.h"
#include "p_console.h"
#include "p_calc_stack.h"

// C++
#include <sstream>

// STL
// none

// Qt
#include <QImage>
#include <QRgb>

/** \file pvirtualmachine.cpp
 * \brief Plik z kodem źródłowym klasy PVirtualMachine
 *
 * Plik zawiera kod źródłowy klasy PVirtualMachine.
 */

/**
 * Konstruktor maszyny wirtualnej interpretującej dowolny program w języku Piet. Tworzy wszystkie pomocnicze obiekty których działanie jest wykorzystywane i koordynowane przez wirtualną maszynę.
 */
PVirtualMachine::PVirtualMachine(const char * filename, std::stringstream &str) : stream(str)
{
	debug("CONSTRUCTOR - virtual-machine START\n");

	verbose = false;

	// stworzenie obiektu obrazu kodu, po którym będzie poruszać się głowica

	image = new QImage(filename);

	// punkt współrzędnych początkowych - oraz stworzenie głowicy poruszającej się po powyższym obrazie
	PPoint initial;
	initial.x = initial.y = 0;
	pointer = new PCodePointer(image, initial);

	// obiekt odpowiedzialny za geometryczną interpretację bloków kolorów
	block_manager = new PBlockManager(image, pointer, str);

	// obiekt przetwarzający wszystko związane z kolorami
	color_manager = new PColorManager(str);

	// stos przechowujący tymczasowe wartości
	stack = new PCalcStack();

	// obiekt odpowiedzialny za wczytywanie i wyświetlanie danych od użytkownika
	console = new PConsole(str);

	// przygotowanie maszyny do uruchomienia ("wyzerowanie")
	prepareToExecute();

	debug("CONSTRUCTOR - virtual-machine END\n");
}

/**
 * Destruktor wirtualnej maszyny Pieta. Wywołuje destruktory dla pomocniczych obiektów tworzonych w konstruktorze wirtualnej maszyny.
 */
PVirtualMachine::~PVirtualMachine()
{
	debug("DESTRUCTOR - virtual-machine START\n");
	// likwidacja kolejnych pomocniczych obiektów używanych przez wirtualną maszynę
	delete image;
	delete pointer;
	delete block_manager;
	delete color_manager;
	delete stack;
	delete console;
	debug("DESTRUCTOR - virtual-machine END\n");
}

/** \brief Przygotowanie do uruchomienia/zresetowania wirtualnej maszyny
 *
 * Metoda używana w celu przygotowania wirtualnej maszyny do uruchomienia. Maszyna mogła być w trakcie działania, mogła zakończyć działanie lub mogła być gotowa do uruchomienia.
 */
void PVirtualMachine::prepareToExecute()
{
	stack->clear();
	pointer->clear();
	setState(state_ready);
	step = 0;
}

//=============================================================================

/** \brief zwraca stan wirtualnej maszyny Salvadora
 *
 * W każdej chwili istnienia, wirtualna maszyna Pieta musi znajdować się w jakimś stanie (PMachineStates). Metoda zwraca aktualny stan wirtualnej maszyny.
 */
PMachineStates PVirtualMachine::getState()
{
	return state;
}

/** \brief ustawia stan wirtualnej maszyny Salvadora
 *
 * W każdej chwili istnienia, wirtualna maszyna Pieta musi znajdować się w jakimś stanie (PMachineStates). Metoda ustawia stan wirtualnej maszyny.
 * \param state_ stan jaki zostanie przypisany wirtualnej maszynie
 */
void PVirtualMachine::setState(PMachineStates state_)
{
	state = state_;
}

/**
 * Sprawdza czy maszyna jest gotowa do rozpoczęcia pracy (sprawdzany stan maszyny).
 * @return czy maszyna jest gotowa do uruchomienia
 */
bool PVirtualMachine::isReady()
{
	return (state == state_ready);
}

/**
 * Sprawdza czy maszyna pracuje (sprawdzany stan maszyny).
 * @return czy maszyna pracuje
 */
bool PVirtualMachine::isRunning()
{
	return (state == state_running);
}

/**
 * Sprawdza czy maszyna zakończyła działanie (sprawdzany stan maszyny).
 * @return czy maszyna zakończyła działanie
 */
bool PVirtualMachine::isFinished()
{
	return (state == state_finished);
}

//=========================================================

/**
 * Uruchamia maszynę (ustawia odpowiedni stan).
 * @return czy operacja się powiodła
 */
bool PVirtualMachine::startMachine()
{
	if (isReady()) {
		setState(state_running);
		return true;
	} else {
		return false;
	}
}

/**
 * Restartuje maszynę (ustawia odpowiedni stan).
 * @return czy operacja się powiodła
 */
bool PVirtualMachine::restartMachine()
{
	prepareToExecute();
	return true;
}

/**
 * Zatrzymuje maszynę (ustawia odpowiedni stan).
 * @return czy operacja się powiodła
 */
bool PVirtualMachine::stopMachine()
{
	if (isRunning()) {
		setState(state_finished);
		return true;
	} else {
		return false;
	}
}

//=========================================================

void PVirtualMachine::setVerbosityRecursively(bool verbosity)
{
	verbose = verbosity;
	console->setVerbosity(verbosity);
	color_manager->setVerbosity(verbosity);
	block_manager->setVerbosity(verbosity);
	stack->setVerbosity(verbosity);
	pointer->setVerbosity(verbosity);
}

/**
 * Sprawdza, czy maszyna ma włączony tryb gadatliwy
 * @return tryb gadatliwy
 */
bool PVirtualMachine::isVerbose()
{
	return verbose;
}

/**
 * Przełącza tryb gadatliwy na przeciwny. Przełącza tryb we wszystkich podrzędnych obiektach.
 */
void PVirtualMachine::toggleVerbosity()
{
	if (verbose) {
		setVerbosityRecursively(false);
	} else {
		setVerbosityRecursively(true);
	}
}

void PVirtualMachine::setVerbosity(bool verbosity)
{
	setVerbosityRecursively(verbosity);
}

//=========================================================

std::list<int>::iterator PVirtualMachine::calc_stack_begin_iterator()
{
	return stack->begin_iterator();
}

std::list<int>::iterator PVirtualMachine::calc_stack_end_iterator()
{
	return stack->end_iterator();
}

//=========================================================

const QImage* PVirtualMachine::getImage() const
{
	return this->image;
}

//=========================================================

/**
 * Wykonuje WSZYSTKIE instrukcje aż do zakończenia pracy programu. Jeśli tryb gadatliwy został uprzednio włączony, wszystkie informacje o przebeigu pracy są wyświetlane.
 */
void PVirtualMachine::executeAllInstr()
{
	if (verbose) {
		__dev__printImageInfo();
	}
	while ( isRunning() ) {
		executeSingleInstr();
	}
}

/**
 * Wykonuje POJEDYNCZĄ instrukcję aż do zakończenia pracy programu. Jeśli tryb gadatliwy został uprzednio włączony, wszystkie informacje o przebeigu pracy są wyświetlane.
 */
bool PVirtualMachine::executeSingleInstr()
{
	if (verbose) {
		stream << std::endl << "krok:" << step << "; ";
		__dev__printConsole();
	}
	bool result = true; // czy operacja została wykonana
	if ( isRunning() ) {
		PInstructions instruction = movePointerAndGetInstructionToExecute();
		if (verbose) {
			stream << "instr: " << PEnums::instruction(instruction) << std::endl;
		}
		// w zależności od tego jaką instrukcję zwróci maszyna kodu
		switch(instruction)
		{
		// wykonaj inną operację używając stosu
			case pietInstr_special_empty:
				// pusta instrukcja - głowica przesunięta po białym bloku
				break;
			case pietInstr_stack_push:
				stack->instrPush( block_manager->getCodelBlockCount() ); // połóż na stosie liczbę równą liczbie kodeli w bloku kolorów wskazywanym aktualnie przez głowicę
				break;
			case pietInstr_stack_pop:
				if (stack->hasAtLeastNElements(1)) {
					stack->instrPop(); // zdejmuje ze stosu liczbę (i nic z nią nie robi)
				} else {
					result = false;
				}
				break;
			case pietInstr_arithm_add:
				if (stack->hasAtLeastNElements(2)) {
					stack->instrAdd(); // zdejmuje 2 szczytowe elementy i odkłada ich sumę
				} else {
					result = false;
				}
				break;
			case pietInstr_arithm_subtract:
				if (stack->hasAtLeastNElements(2)) {
					stack->instrSubtract(); // zdejmuje 2 szczytowe elementy i odkłada ich różnicę
				} else {
					result = false;
				}
				break;
			case pietInstr_arithm_multiply:
				if (stack->hasAtLeastNElements(2)) {
					stack->instrMultiply(); // zdejmuje 2 szczytowe elementy i odkłada ich iloczyn
				} else {
					result = false;
				}
				break;
			case pietInstr_arithm_divide:
				if (stack->hasAtLeastNElements(2)) {
					stack->instrDivide(); // zdejmuje 2 szczytowe elementy i odkłada ich iloraz
				} else {
					result = false;
				}
				break;
			case pietInstr_arithm_modulo:
				if (stack->hasAtLeastNElements(2)) {
					stack->instrModulo(); // zdejmuje 2 szczytowe elementy i odkłada resztę z ich dzielenia
				} else {
					result = false;
				}
				break;
			case pietInstr_logic_not:
				if (stack->hasAtLeastNElements(1)) {
					stack->instrNot(); // zdejmuje element szczytowy i odkłada 1 (el = 0) lub 0 (el != 0)
				} else {
					result = false;
				}
				break;
			case pietInstr_logic_greater:
				if (stack->hasAtLeastNElements(2)) {
					stack->instrGreater(); // zdejmuje 2 szczytowe elementy i odkłada 1 (jeśli ostatni większy) lub 0 (przeciwnie)
				} else {
					result = false;
				}
				break;
			case pietInstr_runtime_pointer:
				if (stack->hasAtLeastNElements(1)) {
					for (int num = stack->instrPop(); num > 0; num--)
						pointer->toggleDirectionPointer();
				} else {
					result = false;
				}
				break;
			case pietInstr_runtime_switch:
				if (stack->hasAtLeastNElements(1)) {
					if (stack->instrPop() % 2)
						pointer->toggleCodelChooser();
				} else {
					result = false;
				}
				break;
			case pietInstr_stack_duplicate:
				if (stack->hasAtLeastNElements(1)) {
					stack->instrDuplicate();
				} else {
					result = false;
				}
				break;
			case pietInstr_stack_roll:
				if (stack->hasAtLeastNElements(2)) {
					stack->instrRoll();
				} else {
					result = false;
				}
				break;
			case pietInstr_io_in_number:
				readNumber();
				break;
			case pietInstr_io_in_char:
				readChar();
				break;
			case pietInstr_io_out_number:
				if (stack->hasAtLeastNElements(1)) {
					console->printNumber(stack->instrPop());
				} else {
					result = false;
				}
				break;
			case pietInstr_io_out_char:
				if (stack->hasAtLeastNElements(1)) {
					console->printChar(stack->instrPop());
				} else {
					result = false;
				}
				break;
			case pietInstr_special_terminate:
				// 8 nieudanych prób przesunięcia głowicy do następnego kodela kończy pracę interpretera
				if ( !stopMachine() ) {
					stream << "ERROR: bool PVirtualMachine::executeSingleInstr()" << std::endl;
					exit(1);
				}
				break;
			default:
				stream << "ERROR: bool PVirtualMachine::executeSingleInstr()" << std::endl;
				exit(1);
		}
	} else {
		result = false;
	}
	step++;
	return result;
}

//==================================================================

/**
 * Sprawdza, czy wskazany punkt jest koloru czarnego lub się znajduje poza granicami obrazu kodu. Jeśli tak, głowica będzie musiała zmienić swoje DP i/lub CC. Wirtualna maszyna wykorzystuje do tego głowicę obrazu kodu oraz menadżera kolorów.
 * @param point wskazany punkt
 * @return czy punkt jest czarny lub poza granicami obrazu kodu
 */
bool PVirtualMachine::pointIsBlackOrOutside(PPoint point)
{
	bool black = false, outside = pointer->pointOutsideImage(point);
	if (!outside) {
		black = (color_manager->getColorName(pointer->getPixel(point)) == color_black );
	}
	return ( outside || black );
}

/**
 * Sprawdza, czy wskazany punkt jest koloru białego. Jeśli tak, to jeśli głowica dodatkowo prześlizgnie się przez biały blok, nie wykona w tym krokużadnej operacji. Wirtualna maszyna wykorzystuje do tego głowicę obrazu kodu oraz menadżera kolorów.
 * @param point wskazany punkt
 * @return czy punkt jest biały
 */
bool PVirtualMachine::pointIsWhite(PPoint point)
{
	return (color_manager->getColorName(pointer->getPixel(point)) == color_white );
}

/**
 * Przesuwa głowicę o jeden kodel przez biały blok. Metoda wywoływana wielokrotnie przez metodę slideAcrossWhiteBlock().
 */
void PVirtualMachine::slidePointerAcrossWhiteBlock()
{
	while (color_manager->getColorName(pointer->getPointedPixel()) == color_white)
	switch (pointer->getDirectionPointerValue()) {
		case dp_right:
			pointer->incCoordinateX();
			break;
		case dp_down:
			pointer->incCoordinateY();
			break;
		case dp_left:
			pointer->decCoordinateX();
			break;
		case dp_up:
			pointer->decCoordinateY();
			break;
	}
}

/**
 * Przesuwa głowicę, uwzględniając jej wartości DP i CC, na koniec białego bloku (parametr point, przekazywany przez zmienną - ulega zmianie w trakcie wykonywania operacji). Następnie wirtualna maszyna bada, czy otrzymany punkt znajduje się poza obrazem kodu lub jest czarny (wtedy głowica zmienia wartości DP i/lub CC i być może ponownie będzie wędrowała z wyjściowego punktu przez biały blok).
 * @param point wskazany punkt
 */
void PVirtualMachine::slideAcrossWhiteBlock(PPoint &point)
{
	while ( (!pointer->pointOutsideImage(point)) && (color_manager->getColorName(pointer->getPixel(point)) == color_white) )
	switch (pointer->getDirectionPointerValue()) {
		case dp_right:
			point.x++;
			break;
		case dp_down:
			point.y++;
			break;
		case dp_left:
			point.x--;
			break;
		case dp_up:
			point.y--;
			break;
	}
}

/**
 * Metoda nakazuje przesunięcie głowicy oraz wyznacza jaka instrukcja ma zostać wykonana (wszystko wykorzystując pomocnicze obiekty wirtualnej maszyny). Jedna z ważniejszych i bardziej skomplikowanych metod całego interpretera.
 */
PInstructions PVirtualMachine::movePointerAndGetInstructionToExecute()
{
	PInstructions result_instr; // robocza zmienna wynikowa
	PPoint possible_point; // nowy, szukany kodel na który ma zostać przesunięta głowica
	QRgb old_color = pointer->getPointedPixel(); // kolor pierwotnego kodela, czyli wskazywanego przez głowicę przed jej przesunięciem
	block_manager->searchAndFillCodels(); // oblicza liczbę kodeli bloku koloru w którym się znajduje kodel wskazywany przez głowicę
	bool isWhite = false, isBlack;

	if (verbose)
		block_manager->__dev__showCountAndBorderCodels();

	int attempts = 0; // licznik prób wykonania przez głowicę ruchu (patrz: specyfikacja, "czarne bloki i granice") Jeśli po 8 próbach nie uda się wykonać ruchu, program kończy działanie.
	bool continued = true; // zmienna sterująca pętlą
	while (continued) {
		isBlack = isWhite = false; // odznaczenie informacji na początek każdego obrotu pętli (odświeżenie)
		possible_point = block_manager->getNextPossibleCodel(); // wyznacz hipotetyczny nowy kodel
		if (verbose) {
			stream << "try:" << attempts << "[" << possible_point.x << "," << possible_point.y << "]; ";
		}

		if ( pointIsBlackOrOutside(possible_point) ) {
			isBlack = true;
		} else if ( pointIsWhite(possible_point) ) {
			slideAcrossWhiteBlock(possible_point);
			isWhite = true;
			if ( pointIsBlackOrOutside(possible_point) ) {
				isBlack = true;
			}
		}

		if (isBlack) {
			// nie udało się
			attempts++;
			if (attempts % 2) {
				pointer->toggleCodelChooser();
			} else {
				pointer->toggleDirectionPointer();
			}
			continued = (attempts < 8);
		} else {
			pointer->setCoordinates(possible_point);
			if (verbose)
				stream << "new:[" << possible_point.x << "," << possible_point.y << "]" << "; ";
			continued = false;
		}
	}
	// pętla zerwana - albo przesunięto głowicę albo program zakończony, poniżej selekcja
	if (attempts == 8) {
		result_instr = pietInstr_special_terminate; // zakończenie programu, 8 nieudanych prób
	} else if (isWhite) {
		result_instr = pietInstr_special_empty;
	} else { // głowica została przesunięta
		QRgb new_color = pointer->getPointedPixel(); // odczytanie koloru kodelu wskazywanego przez głowicę po przesunięciu
		result_instr = getInstructionByIndex( color_manager->getInstructionIndex(old_color, new_color) ); // interpretacja instrukcji do wykonanai na podstawie kolorów kodeli - starego i nowego
	}
	return result_instr; // zwrócenie wyniku (instrukcji do wykonania)
}

//==================================================================

/**
 * Zwraca instrukcję Pieta (element enumeracji) dla zadanego indeksu instrukcji Pieta.
 * @param index indeks instrukcji Pieta
 * @return instrukcja Pieta
 */
PInstructions PVirtualMachine::getInstructionByIndex(int index)
{
	switch(index) {
		case 0:
			return pietInstr_special_empty;
		case 1:
			return pietInstr_stack_push;
		case 2:
			return pietInstr_stack_pop;
		case 3:
			return pietInstr_arithm_add;
		case 4:
			return pietInstr_arithm_subtract;
		case 5:
			return pietInstr_arithm_multiply;
		case 6:
			return pietInstr_arithm_divide;
		case 7:
			return pietInstr_arithm_modulo;
		case 8:
			return pietInstr_logic_not;
		case 9:
			return pietInstr_logic_greater;
		case 10:
			return pietInstr_runtime_pointer;
		case 11:
			return pietInstr_runtime_switch;
		case 12:
			return pietInstr_stack_duplicate;
		case 13:
			return pietInstr_stack_roll;
		case 14:
			return pietInstr_io_in_number;
		case 15:
			return pietInstr_io_in_char;
		case 16:
			return pietInstr_io_out_number;
		case 17:
			return pietInstr_io_out_char;
		case 18:
			return pietInstr_special_terminate;
		default:
			stream << "ERROR: PInstructions PVirtualMachine::getInstructionByIndex(int index)" << std::endl;
			exit(1);
	}
}

PPoint PVirtualMachine::getCodePointerCoordinates()
{
	return pointer->getCoordinates();
}

PDirectionPointerValues PVirtualMachine::getDirectionPointer()
{
	return pointer->getDirectionPointerValue();
}

PCodelChooserValues PVirtualMachine::getCodelChooser()
{
	return pointer->getCodelChooserValue();
}

/**
 * METODA TESTOWA. Wyświetla informacje o obrazie kodu.
 */
void PVirtualMachine::__dev__printImageInfo()
{
	stream << std::endl << "CODE IMAGE/ " << "[" << image->width() << "x" << image->height() << "]" << std::endl;
}

/**
 * METODA TESTOWA. Wyświetla informacje o całej maszynie i jej elementach składowych.
 */
void PVirtualMachine::__dev__printConsole()
{
	stream << "CODE POINTER/";

	PPoint coords = pointer->getCoordinates();
	stream << "[" << coords.x << "," << coords.y << "]";

	PDirectionPointerValues dp = pointer->getDirectionPointerValue();
	stream << "DP:" << PEnums::directionPointer(dp) << "(" << (int) dp << ")";

	PCodelChooserValues cc = pointer->getCodelChooserValue();
	stream << "CC:" << PEnums::codelChooser(cc) << "(" << (int) cc << ")";

	stream << std::endl << "DATA STACK/";
	stream << "rozm:" << stack->size() << " elem: ";
	for (std::list<int>::iterator it = stack->begin_iterator(); it != stack->end_iterator(); ++it) {
		stream << *it << " ";
	}
	stream << std::endl;
}
