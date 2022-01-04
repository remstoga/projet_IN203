// simulation_sync_affiche_mpi.cpp (peut-être renommé)

#include <cstdlib>
#include <random>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mpi.h>
#include "contexte.hpp"
#include "individu.hpp"
#include "graphisme/src/SDL2/sdl2.hpp"

void màjStatistique(épidémie::Grille &grille, std::vector<épidémie::Individu> const &individus)
{
    for (auto &statistique : grille.getStatistiques())
    {
        statistique.nombre_contaminant_grippé_et_contaminé_par_agent = 0;
        statistique.nombre_contaminant_seulement_contaminé_par_agent = 0;
        statistique.nombre_contaminant_seulement_grippé = 0;
    }
    auto [largeur, hauteur] = grille.dimension();
    auto &statistiques = grille.getStatistiques();
    for (auto const &personne : individus)
    {
        auto pos = personne.position();

        std::size_t index = pos.x + pos.y * largeur;
        if (personne.aGrippeContagieuse())
        {
            if (personne.aAgentPathogèneContagieux())
            {
                statistiques[index].nombre_contaminant_grippé_et_contaminé_par_agent += 1;
            }
            else
            {
                statistiques[index].nombre_contaminant_seulement_grippé += 1;
            }
        }
        else
        {
            if (personne.aAgentPathogèneContagieux())
            {
                statistiques[index].nombre_contaminant_seulement_contaminé_par_agent += 1;
            }
        }
    }
}

void afficheSimulation(sdl2::window &écran, épidémie::Grille const &grille, std::size_t jour)
{
    auto [largeur_écran, hauteur_écran] = écran.dimensions();
    auto [largeur_grille, hauteur_grille] = grille.dimension();
    auto const &statistiques = grille.getStatistiques();
    sdl2::font fonte_texte("./graphisme/src/data/Lato-Thin.ttf", 18);
    écran.cls({0x00, 0x00, 0x00});
    // Affichage de la grille :
    std::uint16_t stepX = largeur_écran / largeur_grille;
    unsigned short stepY = (hauteur_écran - 50) / hauteur_grille;
    double factor = 255. / 15.;

    for (unsigned short i = 0; i < largeur_grille; ++i)
    {
        for (unsigned short j = 0; j < hauteur_grille; ++j)
        {
            auto const &stat = statistiques[i + j * largeur_grille];
            int valueGrippe = stat.nombre_contaminant_grippé_et_contaminé_par_agent + stat.nombre_contaminant_seulement_grippé;
            int valueAgent = stat.nombre_contaminant_grippé_et_contaminé_par_agent + stat.nombre_contaminant_seulement_contaminé_par_agent;
            std::uint16_t origx = i * stepX;
            std::uint16_t origy = j * stepY;
            std::uint8_t red = valueGrippe > 0 ? 127 + std::uint8_t(std::min(128., 0.5 * factor * valueGrippe)) : 0;
            std::uint8_t green = std::uint8_t(std::min(255., factor * valueAgent));
            std::uint8_t blue = std::uint8_t(std::min(255., factor * valueAgent));
            écran << sdl2::rectangle({origx, origy}, {stepX, stepY}, {red, green, blue}, true);
        }
    }

    écran << sdl2::texte("Carte population grippée", fonte_texte, écran, {0xFF, 0xFF, 0xFF, 0xFF}).at(largeur_écran / 2, hauteur_écran - 20);
    écran << sdl2::texte(std::string("Jour : ") + std::to_string(jour), fonte_texte, écran, {0xFF, 0xFF, 0xFF, 0xFF}).at(0, hauteur_écran - 20);
    écran << sdl2::flush;
}

void place_in_buffer(std::vector<épidémie::Grille::StatistiqueParCase> &statistiques, int *buffer)
{
    int index = 0;
    for (const auto &value : statistiques)
    {
        buffer[index] = value.nombre_contaminant_seulement_grippé;
        buffer[index + 1] = value.nombre_contaminant_seulement_contaminé_par_agent;
        buffer[index + 2] = value.nombre_contaminant_grippé_et_contaminé_par_agent;
        index += 3;
    }
}

void recv_from_buffer(std::vector<épidémie::Grille::StatistiqueParCase> &statistiques, int *buffer)
{
    int index = 0;
    for (auto &value : statistiques)
    {
        value.nombre_contaminant_seulement_grippé = buffer[index];
        value.nombre_contaminant_seulement_contaminé_par_agent = buffer[index + 1];
        value.nombre_contaminant_grippé_et_contaminé_par_agent = buffer[index + 2];
        index += 3;
    }
}

void simulation(bool affiche)
{

    // Affectation des paramètres MPI
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    épidémie::ContexteGlobal contexte;
    épidémie::Grille grille{contexte.taux_population};
    std::size_t jours_écoulés = 0;

    if (rank == 1)
    {

        unsigned int graine_aléatoire = 1;
        std::uniform_real_distribution<double> porteur_pathogène(0., 1.);
        // contexte.déplacement_maximal = 1; <= Si on veut moins de brassage
        // contexte.taux_population = 400'000;
        // contexte.taux_population = 1'000;
        contexte.interactions.β = 60.;
        std::vector<épidémie::Individu> population;
        population.reserve(contexte.taux_population);

        auto [largeur_grille, hauteur_grille] = grille.dimension();
        // L'agent pathogène n'évolue pas et reste donc constant...
        épidémie::AgentPathogène agent(graine_aléatoire++);
        // Initialisation de la population initiale :
        for (std::size_t i = 0; i < contexte.taux_population; ++i)
        {
            std::default_random_engine motor(100 * (i + 1));
            population.emplace_back(graine_aléatoire++, contexte.espérance_de_vie, contexte.déplacement_maximal);
            population.back().setPosition(largeur_grille, hauteur_grille);
            if (porteur_pathogène(motor) < 0.2)
            {
                population.back().estContaminé(agent);
            }
        }

        int jour_apparition_grippe = 0;
        int nombre_immunisés_grippe = (contexte.taux_population * 23) / 100;

        bool quitting = false;

        std::ofstream output("Courbe.dat");
        output << "# jours_écoulés \t nombreTotalContaminésGrippe \t nombreTotalContaminésAgentPathogène()" << std::endl;

        épidémie::Grippe grippe(0);

        std::cout << "Début boucle épidémie" << std::endl
                  << std::flush;

        while (!quitting)
        {

            std::chrono::steady_clock::time_point t_debut = std::chrono::steady_clock::now();

            if (jours_écoulés % 365 == 0) // Si le premier Octobre (début de l'année pour l'épidémie ;-) )
            {
                grippe = épidémie::Grippe(jours_écoulés / 365);
                jour_apparition_grippe = grippe.dateCalculImportationGrippe();
                grippe.calculNouveauTauxTransmission();
                // 23% des gens sont immunisés. On prend les 23% premiers
                for (int ipersonne = 0; ipersonne < nombre_immunisés_grippe; ++ipersonne)
                {
                    population[ipersonne].devientImmuniséGrippe();
                }
                for (int ipersonne = nombre_immunisés_grippe; ipersonne < int(contexte.taux_population); ++ipersonne)
                {
                    population[ipersonne].redevientSensibleGrippe();
                }
            }
            if (jours_écoulés % 365 == std::size_t(jour_apparition_grippe))
            {
                for (int ipersonne = nombre_immunisés_grippe; ipersonne < nombre_immunisés_grippe + 25; ++ipersonne)
                {
                    population[ipersonne].estContaminé(grippe);
                }
            }
            // Mise à jour des statistiques pour les cases de la grille :
            màjStatistique(grille, population);
            // On parcout la population pour voir qui est contaminé et qui ne l'est pas, d'abord pour la grippe puis pour l'agent pathogène
            std::size_t compteur_grippe = 0, compteur_agent = 0, mouru = 0;
            for (auto &personne : population)
            {
                if (personne.testContaminationGrippe(grille, contexte.interactions, grippe, agent))
                {
                    compteur_grippe++;
                    personne.estContaminé(grippe);
                }
                if (personne.testContaminationAgent(grille, agent))
                {
                    compteur_agent++;
                    personne.estContaminé(agent);
                }
                // On vérifie si il n'y a pas de personne qui veillissent de veillesse et on génère une nouvelle personne si c'est le cas.
                if (personne.doitMourir())
                {
                    mouru++;
                    unsigned nouvelle_graine = jours_écoulés + personne.position().x * personne.position().y;
                    personne = épidémie::Individu(nouvelle_graine, contexte.espérance_de_vie, contexte.déplacement_maximal);
                    personne.setPosition(largeur_grille, hauteur_grille);
                }
                personne.veillirDUnJour();
                personne.seDéplace(grille);
            }

            //#############################################################################################################
            //##    Affichage des résultats pour le temps  actuel
            //#############################################################################################################

            /*std::cout << jours_écoulés << "\t" << grille.nombreTotalContaminésGrippe() << "\t"
                      << grille.nombreTotalContaminésAgentPathogène() << std::endl;*/

            output << jours_écoulés << "\t" << grille.nombreTotalContaminésGrippe() << "\t"
                   << grille.nombreTotalContaminésAgentPathogène() << std::endl;
            jours_écoulés += 1;
            int send_buffer[hauteur_grille * largeur_grille * 3];
            place_in_buffer(grille.getStatistiques(), send_buffer);
            MPI_Send(send_buffer, hauteur_grille * largeur_grille * 3, MPI_INT, 0, 110, MPI_COMM_WORLD);

            std::chrono::steady_clock::time_point t_fin = std::chrono::steady_clock::now();
            std::cout << "temps d'une boucle : " << std::chrono::duration_cast<std::chrono::microseconds>(t_fin - t_debut).count() << " µs " << std::endl;
        }
        // Fin boucle temporelle
        output.close();
    }

    if (rank == 0)
    {
        constexpr const unsigned int largeur_écran = 1280,
                                     hauteur_écran = 1024;
        sdl2::window écran("Simulation épidémie de grippe", {largeur_écran, hauteur_écran});

        sdl2::event_queue queue;
        auto [largeur_grille, hauteur_grille] = grille.dimension();
        bool quitting = false;

        while (!quitting)
        {
            auto events = queue.pull_events();
            for (const auto &e : events)
            {
                if (e->kind_of_event() == sdl2::event::quit)
                    quitting = true;
            }

            MPI_Status status;
            int recv_buffer[largeur_grille * hauteur_grille * 3];
            MPI_Recv(recv_buffer, largeur_grille * hauteur_grille * 3, MPI_INT, 1, 110, MPI_COMM_WORLD, &status);
            recv_from_buffer(grille.getStatistiques(), recv_buffer);
            std::chrono::steady_clock::time_point t_debut_aff;
            std::chrono::steady_clock::time_point t_fin_aff;

            if (affiche)
            {
                t_debut_aff = std::chrono::steady_clock::now();
                afficheSimulation(écran, grille, jours_écoulés);
                t_fin_aff = std::chrono::steady_clock::now();
            }
            jours_écoulés++;

            if (affiche)
                std::cout << "temps d'affichage : " << std::chrono::duration_cast<std::chrono::microseconds>(t_fin_aff - t_debut_aff).count() << " µs " << std::endl;
        }
    }
}

int main(int argc, char *argv[])
{
    // parse command-line
    bool affiche = true;
    for (int i = 0; i < argc; i++)
    {
        std::cout << i << " " << argv[i] << "\n";
        if (std::string("-nw") == argv[i])
            affiche = false;
    }

    MPI_Init(&argc, &argv);
    sdl2::init(argc, argv);
    {
        simulation(affiche);
    }
    sdl2::finalize();
    MPI_Finalize();
    return EXIT_SUCCESS;
}
