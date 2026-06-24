# WandaGEVK — Vulkan Game Engine Architecture

Projet de développement d'un moteur graphique 2D/3D basé sur l'API **Vulkan 1.2**, réalisé dans le cadre du cursus d'ingénieur à l'**ENSPY** (Génie Informatique / Game Programming). 

L'objectif principal est de concevoir une architecture logicielle modulaire et performante, entièrement découplée de la gestion de fenêtrage native grâce au framework `NKWindow`.

---

## 🚀 État Actuel du Projet

L'initialisation fondamentale de l'infrastructure Vulkan est terminée et validée opérationnelle. Le pipeline gère actuellement :

- [x] **Étape 1 : Initialisation de l'Instance** — Configuration de `VkInstance` en version Vulkan 1.2 avec gestion dynamique des extensions multiplateformes.
- [x] **Validation Layers & Debugging** — Intégration d'un messager de débogage permanent (`VkDebugUtilsMessengerEXT`) routé vers le logger natif.
- [x] **Étape 2 : Sélection du GPU (Physical Device)** — Énumération et sélection automatique du meilleur composant matériel (Priorité Discrete GPU, repli transparent sur Integrated GPU).
- [x] **Étape 4 : Surface de Rendu Native** — Extraction des handles système via `NkSurfaceDesc` (`HWND`, `wl_surface`, etc.) et instanciation manuelle de la `VkSurfaceKHR` pour préserver l'agnosticisme de la fenêtre.
- [x] **Étape 3 : Périphérique Logique (Logical Device)** — Création du `VkDevice` configuré avec les files d'attente requises (Graphics & Present Queues) et l'extension Swapchain.
- [x] **Étape 5 : Swapchain** — Interrogation des capacités matérielles de la surface, sélection du format d'image optimal (SRGB) et repli sécurisé sur le mode `FIFO` si le mode `MAILBOX` n'est pas supporté.
- [x] **Étape 6 : Image Views** — Génération des wrappers `VkImageView` pour chacune des images de la swapchain afin de les rendre exploitables par le pipeline.
- [x] **Gestion de l'arrêt (WaitIdle)** — Nettoyage explicite et ordonné de l'intégralité des ressources graphiques lors de la fermeture de la fenêtre grâce à `vkDeviceWaitIdle`.

---

## 🛠️ Stack Technique

* **Langage :** C++17 / C++20
* **API Graphique :** Vulkan SDK 1.2+
* **Gestion des fenêtres & événements :** NKWindow (Framework interne)
* **Système de Build :** Jenga / Python Configuration Suite

---
