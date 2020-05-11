#pragma once

#include "installables.hh"
#include "args.hh"
#include "common-eval-args.hh"
#include "path.hh"
#include "flake/lockfile.hh"

#include <optional>

namespace nix {

extern std::string programPath;

class EvalState;
struct Pos;
class Store;

static constexpr Command::Category catSecondary = 100;
static constexpr Command::Category catUtility = 101;
static constexpr Command::Category catNixInstallation = 102;

/* A command that requires a Nix store. */
struct StoreCommand : virtual Command
{
    StoreCommand();
    void run() override;
    ref<Store> getStore();
    virtual ref<Store> createStore();
    virtual void run(ref<Store>) = 0;

private:
    std::shared_ptr<Store> _store;
};

struct EvalCommand : virtual StoreCommand, MixEvalArgs
{
    ref<EvalState> getEvalState();

    std::shared_ptr<EvalState> evalState;

    void completeFlakeRef(std::string_view prefix);
};

struct MixFlakeOptions : virtual Args
{
    flake::LockFlags lockFlags;

    MixFlakeOptions();
};

struct SourceExprCommand : virtual Args, EvalCommand, MixFlakeOptions
{
    std::optional<Path> file;
    std::optional<std::string> expr;

    SourceExprCommand();

    std::vector<std::shared_ptr<Installable>> parseInstallables(
        ref<Store> store, std::vector<std::string> ss);

    std::shared_ptr<Installable> parseInstallable(
        ref<Store> store, const std::string & installable);

    virtual Strings getDefaultFlakeAttrPaths();

    virtual Strings getDefaultFlakeAttrPathPrefixes();

    void completeInstallable(std::string_view prefix);
};

enum RealiseMode { Build, NoBuild, DryRun };

/* A command that operates on a list of "installables", which can be
   store paths, attribute paths, Nix expressions, etc. */
struct InstallablesCommand : virtual Args, SourceExprCommand
{
    std::vector<std::shared_ptr<Installable>> installables;

    InstallablesCommand();

    void prepare() override;

    virtual bool useDefaultInstallables() { return true; }

private:

    std::vector<std::string> _installables;
};

/* A command that operates on exactly one "installable" */
struct InstallableCommand : virtual Args, SourceExprCommand
{
    std::shared_ptr<Installable> installable;

    InstallableCommand();

    void prepare() override;

private:

    std::string _installable{"."};
};

/* A command that operates on zero or more store paths. */
struct StorePathsCommand : public InstallablesCommand
{
private:

    bool recursive = false;
    bool all = false;

protected:

    RealiseMode realiseMode = NoBuild;

public:

    StorePathsCommand(bool recursive = false);

    using StoreCommand::run;

    virtual void run(ref<Store> store, std::vector<StorePath> storePaths) = 0;

    void run(ref<Store> store) override;

    bool useDefaultInstallables() override { return !all; }
};

/* A command that operates on exactly one store path. */
struct StorePathCommand : public InstallablesCommand
{
    using StoreCommand::run;

    virtual void run(ref<Store> store, const StorePath & storePath) = 0;

    void run(ref<Store> store) override;
};

/* A helper class for registering commands globally. */
struct RegisterCommand
{
    static Commands * commands;

    RegisterCommand(const std::string & name,
        std::function<ref<Command>()> command)
    {
        if (!commands) commands = new Commands;
        commands->emplace(name, command);
    }
};

template<class T>
static RegisterCommand registerCommand(const std::string & name)
{
    return RegisterCommand(name, [](){ return make_ref<T>(); });
}

Buildables build(ref<Store> store, RealiseMode mode,
    std::vector<std::shared_ptr<Installable>> installables);

std::set<StorePath> toStorePaths(ref<Store> store, RealiseMode mode,
    std::vector<std::shared_ptr<Installable>> installables);

StorePath toStorePath(ref<Store> store, RealiseMode mode,
    std::shared_ptr<Installable> installable);

std::set<StorePath> toDerivations(ref<Store> store,
    std::vector<std::shared_ptr<Installable>> installables,
    bool useDeriver = false);

/* Helper function to generate args that invoke $EDITOR on
   filename:lineno. */
Strings editorFor(const Pos & pos);

struct MixProfile : virtual StoreCommand
{
    std::optional<Path> profile;

    MixProfile();

    /* If 'profile' is set, make it point at 'storePath'. */
    void updateProfile(const StorePath & storePath);

    /* If 'profile' is set, make it point at the store path produced
       by 'buildables'. */
    void updateProfile(const Buildables & buildables);
};

struct MixDefaultProfile : MixProfile
{
    MixDefaultProfile();
};

struct MixEnvironment : virtual Args {

    StringSet keep, unset;
    Strings stringsEnv;
    std::vector<char*> vectorEnv;
    bool ignoreEnvironment;

    MixEnvironment();

    /* Modify global environ based on ignoreEnvironment, keep, and unset. It's expected that exec will be called before this class goes out of scope, otherwise environ will become invalid. */
    void setEnviron();
};

}
